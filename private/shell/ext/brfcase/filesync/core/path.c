/*
 * path.c - Path ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "volume.h"


/* Constants
 ************/

/* PATHLIST PTRARRAY allocation parameters */

#define NUM_START_PATHS          (32)
#define NUM_PATHS_TO_ADD         (32)

/* PATHLIST string table allocation parameters */

#define NUM_PATH_HASH_BUCKETS    (67)


/* Types
 ********/

/* path list */

typedef struct _pathlist
{
   /* array of pointers to PATHs */

   HPTRARRAY hpa;

   /* list of volumes */

   HVOLUMELIST hvl;

   /* table of path suffix strings */

   HSTRINGTABLE hst;
}
PATHLIST;
DECLARE_STANDARD_TYPES(PATHLIST);

/* path structure */

typedef struct _path
{
   /* reference count */

   ULONG ulcLock;

   /* handle to parent volume */

   HVOLUME hvol;

   /* handle to path suffix string */

   HSTRING hsPathSuffix;

   /* pointer to PATH's parent PATHLIST */

   PPATHLIST pplParent;
}
PATH;
DECLARE_STANDARD_TYPES(PATH);

/* PATH search structure used by PathSearchCmp() */

typedef struct _pathsearchinfo
{
   HVOLUME hvol;

   LPCTSTR pcszPathSuffix;
}
PATHSEARCHINFO;
DECLARE_STANDARD_TYPES(PATHSEARCHINFO);

/* database path list header */

typedef struct _dbpathlistheader
{
   /* number of paths in list */

   LONG lcPaths;
}
DBPATHLISTHEADER;
DECLARE_STANDARD_TYPES(DBPATHLISTHEADER);

/* database path structure */

typedef struct _dbpath
{
   /* old handle to path */

   HPATH hpath;

   /* old handle to parent volume */

   HVOLUME hvol;

   /* old handle to path suffix string */

   HSTRING hsPathSuffix;
}
DBPATH;
DECLARE_STANDARD_TYPES(DBPATH);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE COMPARISONRESULT PathSortCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT PathSearchCmp(PCVOID, PCVOID);
PRIVATE_CODE BOOL UnifyPath(PPATHLIST, HVOLUME, LPCTSTR, PPATH *);
PRIVATE_CODE BOOL CreatePath(PPATHLIST, HVOLUME, LPCTSTR, PPATH *);
PRIVATE_CODE void DestroyPath(PPATH);
PRIVATE_CODE void UnlinkPath(PCPATH);
PRIVATE_CODE void LockPath(PPATH);
PRIVATE_CODE BOOL UnlockPath(PPATH);
PRIVATE_CODE PATHRESULT TranslateVOLUMERESULTToPATHRESULT(VOLUMERESULT);
PRIVATE_CODE TWINRESULT WritePath(HCACHEDFILE, PPATH);
PRIVATE_CODE TWINRESULT ReadPath(HCACHEDFILE, PPATHLIST, HHANDLETRANS, HHANDLETRANS, HHANDLETRANS);

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL IsValidPCPATHLIST(PCPATHLIST);
PRIVATE_CODE BOOL IsValidPCPATH(PCPATH);

#endif

#if defined(DEBUG)

PRIVATE_CODE BOOL IsValidPCPATHSEARCHINFO(PCPATHSEARCHINFO);

#endif


/*
** PathSortCmp()
**
** Pointer comparison function used to sort the module array of paths.
**
** Arguments:     pcpath1 - pointer to first path
**                pcpath2 - pointer to second path
**
** Returns:
**
** Side Effects:  none
**
** The internal paths are sorted by:
**    1) volume
**    2) path suffix
**    3) pointer value
*/
PRIVATE_CODE COMPARISONRESULT PathSortCmp(PCVOID pcpath1, PCVOID pcpath2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcpath1, CPATH));
   ASSERT(IS_VALID_STRUCT_PTR(pcpath2, CPATH));

   cr = CompareVolumes(((PCPATH)pcpath1)->hvol,
                       ((PCPATH)pcpath2)->hvol);

   if (cr == CR_EQUAL)
   {
      cr = ComparePathStringsByHandle(((PCPATH)pcpath1)->hsPathSuffix,
                                      ((PCPATH)pcpath2)->hsPathSuffix);

      if (cr == CR_EQUAL)
         cr = ComparePointers(pcpath1, pcpath2);
   }

   return(cr);
}


/*
** PathSearchCmp()
**
** Pointer comparison function used to search for a path.
**
** Arguments:     pcpathsi - pointer to PATHSEARCHINFO describing path to
**                           search for
**                pcpath - pointer to path to examine
**
** Returns:
**
** Side Effects:  none
**
** The internal paths are searched by:
**    1) volume
**    2) path suffix string
*/
PRIVATE_CODE COMPARISONRESULT PathSearchCmp(PCVOID pcpathsi, PCVOID pcpath)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcpathsi, CPATHSEARCHINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pcpath, CPATH));

   cr = CompareVolumes(((PCPATHSEARCHINFO)pcpathsi)->hvol,
                       ((PCPATH)pcpath)->hvol);

   if (cr == CR_EQUAL)
      cr = ComparePathStrings(((PCPATHSEARCHINFO)pcpathsi)->pcszPathSuffix,
                              GetString(((PCPATH)pcpath)->hsPathSuffix));

   return(cr);
}


/*
** UnifyPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnifyPath(PPATHLIST ppl, HVOLUME hvol, LPCTSTR pcszPathSuffix,
                            PPATH *pppath)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(ppl, CPATHLIST));
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IsValidPathSuffix(pcszPathSuffix));
   ASSERT(IS_VALID_WRITE_PTR(pppath, PPATH));

   /* Allocate space for PATH structure. */

   if (AllocateMemory(sizeof(**pppath), pppath))
   {
      if (CopyVolume(hvol, ppl->hvl, &((*pppath)->hvol)))
      {
         if (AddString(pcszPathSuffix, ppl->hst, GetHashBucketIndex, &((*pppath)->hsPathSuffix)))
         {
            ARRAYINDEX aiUnused;

            /* Initialize remaining PATH fields. */

            (*pppath)->ulcLock = 0;
            (*pppath)->pplParent = ppl;

            /* Add new PATH to array. */

            if (AddPtr(ppl->hpa, PathSortCmp, *pppath, &aiUnused))
               bResult = TRUE;
            else
            {
               DeleteString((*pppath)->hsPathSuffix);
UNIFYPATH_BAIL1:
               DeleteVolume((*pppath)->hvol);
UNIFYPATH_BAIL2:
               FreeMemory(*pppath);
            }
         }
         else
            goto UNIFYPATH_BAIL1;
      }
      else
         goto UNIFYPATH_BAIL2;
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*pppath, CPATH));

   return(bResult);
}


/*
** CreatePath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreatePath(PPATHLIST ppl, HVOLUME hvol, LPCTSTR pcszPathSuffix,
                             PPATH *pppath)
{
   BOOL bResult;
   ARRAYINDEX aiFound;
   PATHSEARCHINFO pathsi;

   ASSERT(IS_VALID_STRUCT_PTR(ppl, CPATHLIST));
   ASSERT(IS_VALID_HANDLE(hvol, VOLUME));
   ASSERT(IsValidPathSuffix(pcszPathSuffix));
   ASSERT(IS_VALID_WRITE_PTR(pppath, CPATH));

   /* Does a path for the given volume and path suffix already exist? */

   pathsi.hvol = hvol;
   pathsi.pcszPathSuffix = pcszPathSuffix;

   bResult = SearchSortedArray(ppl->hpa, &PathSearchCmp, &pathsi, &aiFound);

   if (bResult)
      /* Yes.  Return it. */
      *pppath = GetPtr(ppl->hpa, aiFound);
   else
      bResult = UnifyPath(ppl, hvol, pcszPathSuffix, pppath);

   if (bResult)
      LockPath(*pppath);

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*pppath, CPATH));

   return(bResult);
}


/*
** DestroyPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyPath(PPATH ppath)
{
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   DeleteVolume(ppath->hvol);
   DeleteString(ppath->hsPathSuffix);
   FreeMemory(ppath);

   return;
}


/*
** UnlinkPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void UnlinkPath(PCPATH pcpath)
{
   HPTRARRAY hpa;
   ARRAYINDEX aiFound;

   ASSERT(IS_VALID_STRUCT_PTR(pcpath, CPATH));

   hpa = pcpath->pplParent->hpa;

   if (EVAL(SearchSortedArray(hpa, &PathSortCmp, pcpath, &aiFound)))
   {
      ASSERT(GetPtr(hpa, aiFound) == pcpath);

      DeletePtr(hpa, aiFound);
   }

   return;
}


/*
** LockPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void LockPath(PPATH ppath)
{
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   ASSERT(ppath->ulcLock < ULONG_MAX);
   ppath->ulcLock++;

   return;
}


/*
** UnlockPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnlockPath(PPATH ppath)
{
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   if (EVAL(ppath->ulcLock > 0))
      ppath->ulcLock--;

   return(ppath->ulcLock > 0);
}


/*
** TranslateVOLUMERESULTToPATHRESULT()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE PATHRESULT TranslateVOLUMERESULTToPATHRESULT(VOLUMERESULT vr)
{
   PATHRESULT pr;

   switch (vr)
   {
      case VR_SUCCESS:
         pr = PR_SUCCESS;
         break;

      case VR_UNAVAILABLE_VOLUME:
         pr = PR_UNAVAILABLE_VOLUME;
         break;

      case VR_OUT_OF_MEMORY:
         pr = PR_OUT_OF_MEMORY;
         break;

      default:
         ASSERT(vr == VR_INVALID_PATH);
         pr = PR_INVALID_PATH;
         break;
   }

   return(pr);
}


/*
** WritePath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WritePath(HCACHEDFILE hcf, PPATH ppath)
{
   TWINRESULT tr;
   DBPATH dbpath;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(ppath, CPATH));

   /* Write database path. */

   dbpath.hpath = (HPATH)ppath;
   dbpath.hvol = ppath->hvol;
   dbpath.hsPathSuffix = ppath->hsPathSuffix;

   if (WriteToCachedFile(hcf, (PCVOID)&dbpath, sizeof(dbpath), NULL))
      tr = TR_SUCCESS;
   else
      tr = TR_BRIEFCASE_WRITE_FAILED;

   return(tr);
}


/*
** ReadPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ReadPath(HCACHEDFILE hcf, PPATHLIST ppl,
                                 HHANDLETRANS hhtVolumes,
                                 HHANDLETRANS hhtStrings,
                                 HHANDLETRANS hhtPaths)
{
   TWINRESULT tr;
   DBPATH dbpath;
   DWORD dwcbRead;
   HVOLUME hvol;
   HSTRING hsPathSuffix;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_STRUCT_PTR(ppl, CPATHLIST));
   ASSERT(IS_VALID_HANDLE(hhtVolumes, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtStrings, HANDLETRANS));
   ASSERT(IS_VALID_HANDLE(hhtPaths, HANDLETRANS));

   if (ReadFromCachedFile(hcf, &dbpath, sizeof(dbpath), &dwcbRead) &&
       dwcbRead == sizeof(dbpath) &&
       TranslateHandle(hhtVolumes, (HGENERIC)(dbpath.hvol), (PHGENERIC)&hvol) &&
       TranslateHandle(hhtStrings, (HGENERIC)(dbpath.hsPathSuffix), (PHGENERIC)&hsPathSuffix))
   {
      PPATH ppath;

      if (CreatePath(ppl, hvol, GetString(hsPathSuffix), &ppath))
      {
         /*
          * To leave read paths with 0 initial lock count, we must undo
          * the LockPath() performed by CreatePath().
          */

         UnlockPath(ppath);

         if (AddHandleToHandleTranslator(hhtPaths,
                                         (HGENERIC)(dbpath.hpath),
                                         (HGENERIC)ppath))
            tr = TR_SUCCESS;
         else
         {
            UnlinkPath(ppath);
            DestroyPath(ppath);

            tr = TR_OUT_OF_MEMORY;
         }
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidPCPATHLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCPATHLIST(PCPATHLIST pcpl)
{
   return(IS_VALID_READ_PTR(pcpl, CPATHLIST) &&
          IS_VALID_HANDLE(pcpl->hpa, PTRARRAY) &&
          IS_VALID_HANDLE(pcpl->hvl, VOLUMELIST) &&
          IS_VALID_HANDLE(pcpl->hst, STRINGTABLE));
}


/*
** IsValidPCPATH()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCPATH(PCPATH pcpath)
{
   return(IS_VALID_READ_PTR(pcpath, CPATH) &&
          IS_VALID_HANDLE(pcpath->hvol, VOLUME) &&
          IS_VALID_HANDLE(pcpath->hsPathSuffix, STRING) &&
          IsValidPathSuffix(GetString(pcpath->hsPathSuffix)) &&
          IS_VALID_READ_PTR(pcpath->pplParent, CPATHLIST));
}

#endif


#if defined(DEBUG)

/*
** IsValidPCPATHSEARCHINFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCPATHSEARCHINFO(PCPATHSEARCHINFO pcpathsi)
{
   return(IS_VALID_READ_PTR(pcpathsi, CPATHSEARCHINFO) &&
          IS_VALID_HANDLE(pcpathsi->hvol, VOLUME) &&
          IsValidPathSuffix(pcpathsi->pcszPathSuffix));
}

#endif


/****************************** Public Functions *****************************/


/*
** CreatePathList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreatePathList(DWORD dwFlags, HWND hwndOwner, PHPATHLIST phpl)
{
   BOOL bResult = FALSE;
   PPATHLIST ppl;

   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_RLI_IFLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, RLI_IFL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_PTR(phpl, HPATHLIST));

   if (AllocateMemory(sizeof(*ppl), &ppl))
   {
      NEWPTRARRAY npa;

      /* Create pointer array of paths. */

      npa.aicInitialPtrs = NUM_START_PATHS;
      npa.aicAllocGranularity = NUM_PATHS_TO_ADD;
      npa.dwFlags = NPA_FL_SORTED_ADD;

      if (CreatePtrArray(&npa, &(ppl->hpa)))
      {
         if (CreateVolumeList(dwFlags, hwndOwner, &(ppl->hvl)))
         {
            NEWSTRINGTABLE nszt;

            /* Create string table for path suffix strings. */

            nszt.hbc = NUM_PATH_HASH_BUCKETS;

            if (CreateStringTable(&nszt, &(ppl->hst)))
            {
               *phpl = (HPATHLIST)ppl;
               bResult = TRUE;
            }
            else
            {
               DestroyVolumeList(ppl->hvl);
CREATEPATHLIST_BAIL1:
               DestroyPtrArray(ppl->hpa);
CREATEPATHLIST_BAIL2:
               FreeMemory(ppl);
            }
         }
         else
            goto CREATEPATHLIST_BAIL1;
      }
      else
         goto CREATEPATHLIST_BAIL2;
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phpl, PATHLIST));

   return(bResult);
}


/*
** DestroyPathList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyPathList(HPATHLIST hpl)
{
   ARRAYINDEX aicPtrs;
   ARRAYINDEX ai;

   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));

   /* First free all paths in array. */

   aicPtrs = GetPtrCount(((PCPATHLIST)hpl)->hpa);

   for (ai = 0; ai < aicPtrs; ai++)
      DestroyPath(GetPtr(((PCPATHLIST)hpl)->hpa, ai));

   /* Now wipe out the array. */

   DestroyPtrArray(((PCPATHLIST)hpl)->hpa);

   ASSERT(! GetVolumeCount(((PCPATHLIST)hpl)->hvl));
   DestroyVolumeList(((PCPATHLIST)hpl)->hvl);

   ASSERT(! GetStringCount(((PCPATHLIST)hpl)->hst));
   DestroyStringTable(((PCPATHLIST)hpl)->hst);

   FreeMemory((PPATHLIST)hpl);

   return;
}


/*
** InvalidatePathListInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void InvalidatePathListInfo(HPATHLIST hpl)
{
   InvalidateVolumeListInfo(((PCPATHLIST)hpl)->hvl);

   return;
}


/*
** ClearPathListInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void ClearPathListInfo(HPATHLIST hpl)
{
   ClearVolumeListInfo(((PCPATHLIST)hpl)->hvl);

   return;
}


/*
** AddPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

PUBLIC_CODE PATHRESULT AddPath(HPATHLIST hpl, LPCTSTR pcszPath, PHPATH phpath)
{
   PATHRESULT pr;
   HVOLUME hvol;
   TCHAR rgchPathSuffix[MAX_PATH_LEN];
   LPCTSTR     pszPath;

#ifdef UNICODE
   WCHAR szUnicode[MAX_PATH];
#endif

   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phpath, HPATH));

   // On NT, we want to convert a unicode string to an ANSI shortened path for
   // the sake of interop

   #if defined(UNICODE) 
   {
        CHAR szAnsi[MAX_PATH];
        szUnicode[0] = L'\0';
 
        WideCharToMultiByte(CP_ACP, 0, pcszPath, -1, szAnsi, ARRAYSIZE(szAnsi), NULL, NULL);
        MultiByteToWideChar(CP_ACP, 0, szAnsi,   -1, szUnicode, ARRAYSIZE(szUnicode));
        if (lstrcmp(szUnicode, pcszPath))
        {
            // Cannot convert losslessly from Unicode -> Ansi, so get the short path

            lstrcpy(szUnicode, pcszPath);
            SheShortenPath(szUnicode, TRUE);
            pszPath = szUnicode;
        }
        else
        {
            // It will convert OK, so just use the original

            pszPath = pcszPath;
        }
   }
   #else
        pszPath = pcszPath;
   #endif

   pr = TranslateVOLUMERESULTToPATHRESULT(
            AddVolume(((PCPATHLIST)hpl)->hvl, pszPath, &hvol, rgchPathSuffix));

   if (pr == PR_SUCCESS)
   {
      PPATH ppath;

      if (CreatePath((PPATHLIST)hpl, hvol, rgchPathSuffix, &ppath))
         *phpath = (HPATH)ppath;
      else
         pr = PR_OUT_OF_MEMORY;

      DeleteVolume(hvol);
   }

   ASSERT(pr != PR_SUCCESS ||
          IS_VALID_HANDLE(*phpath, PATH));

   return(pr);
}


/*
** AddChildPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AddChildPath(HPATHLIST hpl, HPATH hpathParent,
                              LPCTSTR pcszSubPath, PHPATH phpathChild)
{
   BOOL bResult;
   TCHAR rgchChildPathSuffix[MAX_PATH_LEN];
   LPCTSTR pcszPathSuffix;
   LPTSTR pszPathSuffixEnd;
   PPATH ppathChild;

   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_HANDLE(hpathParent, PATH));
   ASSERT(IS_VALID_STRING_PTR(pcszSubPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(phpathChild, HPATH));

   ComposePath(rgchChildPathSuffix, 
               GetString(((PCPATH)hpathParent)->hsPathSuffix), 
               pcszSubPath);

   pcszPathSuffix = rgchChildPathSuffix;

   if (IS_SLASH(*pcszPathSuffix))
      pcszPathSuffix++;

   pszPathSuffixEnd = CharPrev(pcszPathSuffix,
                               pcszPathSuffix + lstrlen(pcszPathSuffix));

   if (IS_SLASH(*pszPathSuffixEnd))
      *pszPathSuffixEnd = TEXT('\0');

   ASSERT(IsValidPathSuffix(pcszPathSuffix));

   bResult = CreatePath((PPATHLIST)hpl, ((PCPATH)hpathParent)->hvol,
                        pcszPathSuffix, &ppathChild);

   if (bResult)
      *phpathChild = (HPATH)ppathChild;

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phpathChild, PATH));

   return(bResult);
}


/*
** DeletePath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DeletePath(HPATH hpath)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   if (! UnlockPath((PPATH)hpath))
   {
      UnlinkPath((PPATH)hpath);
      DestroyPath((PPATH)hpath);
   }

   return;
}


/*
** CopyPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CopyPath(HPATH hpathSrc, HPATHLIST hplDest, PHPATH phpathCopy)
{
   BOOL bResult;
   PPATH ppath;

   ASSERT(IS_VALID_HANDLE(hpathSrc, PATH));
   ASSERT(IS_VALID_HANDLE(hplDest, PATHLIST));
   ASSERT(IS_VALID_WRITE_PTR(phpathCopy, HPATH));

   /* Is the destination path list the source path's path list? */

   if (((PCPATH)hpathSrc)->pplParent == (PCPATHLIST)hplDest)
   {
      /* Yes.  Use the source path. */

      LockPath((PPATH)hpathSrc);
      ppath = (PPATH)hpathSrc;
      bResult = TRUE;
   }
   else
      bResult = CreatePath((PPATHLIST)hplDest, ((PCPATH)hpathSrc)->hvol,
                           GetString(((PCPATH)hpathSrc)->hsPathSuffix),
                           &ppath);

   if (bResult)
      *phpathCopy = (HPATH)ppath;

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phpathCopy, PATH));

   return(bResult);
}


/*
** GetPathString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void GetPathString(HPATH hpath, LPTSTR pszPathBuf)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathBuf, STR, MAX_PATH_LEN));

   GetPathRootString(hpath, pszPathBuf);
   CatPath(pszPathBuf, GetString(((PPATH)hpath)->hsPathSuffix));

   ASSERT(IsCanonicalPath(pszPathBuf));

   return;
}


/*
** GetPathRootString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void GetPathRootString(HPATH hpath, LPTSTR pszPathRootBuf)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathRootBuf, STR, MAX_PATH_LEN));

   GetVolumeRootPath(((PPATH)hpath)->hvol, pszPathRootBuf);

   ASSERT(IsCanonicalPath(pszPathRootBuf));

   return;
}


/*
** GetPathSuffixString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void GetPathSuffixString(HPATH hpath, LPTSTR pszPathSuffixBuf)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszPathSuffixBuf, STR, MAX_PATH_LEN));

   ASSERT(lstrlen(GetString(((PPATH)hpath)->hsPathSuffix)) < MAX_PATH_LEN);
   MyLStrCpyN(pszPathSuffixBuf, GetString(((PPATH)hpath)->hsPathSuffix), MAX_PATH_LEN);

   ASSERT(IsValidPathSuffix(pszPathSuffixBuf));

   return;
}


/*
** AllocatePathString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AllocatePathString(HPATH hpath, LPTSTR *ppszPath)
{
   TCHAR rgchPath[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hpath, PATH));
   ASSERT(IS_VALID_WRITE_PTR(ppszPath, LPTSTR));

   GetPathString(hpath, rgchPath);

   return(StringCopy(rgchPath, ppszPath));
}


#ifdef DEBUG

/*
** DebugGetPathString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** N.b., DebugGetPathString() must be non-intrusive.
*/
PUBLIC_CODE LPCTSTR DebugGetPathString(HPATH hpath)
{
#pragma data_seg(DATA_SEG_SHARED)
   /* Allow 4 debug paths. */
   static TCHAR SrgrgchPaths[][MAX_PATH_LEN] = { TEXT(""), TEXT(""), TEXT(""), TEXT("") };
   static UINT SuiPath = 0;
#pragma data_seg()
   LPTSTR pszPath;

   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   pszPath = SrgrgchPaths[SuiPath];

   DebugGetVolumeRootPath(((PPATH)hpath)->hvol, pszPath);
   CatPath(pszPath, GetString(((PPATH)hpath)->hsPathSuffix));

   SuiPath++;
   SuiPath %= ARRAY_ELEMENTS(SrgrgchPaths);

   return(pszPath);
}


/*
** GetPathCount()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE ULONG GetPathCount(HPATHLIST hpl)
{
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));

   return(GetPtrCount(((PCPATHLIST)hpl)->hpa));
}

#endif


/*
** IsPathVolumeAvailable()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsPathVolumeAvailable(HPATH hpath)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   return(IsVolumeAvailable(((PCPATH)hpath)->hvol));
}


/*
** GetPathVolumeID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HVOLUMEID GetPathVolumeID(HPATH hpath)
{
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   return((HVOLUMEID)hpath);
}


/*
** MyIsPathOnVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** MyIsPathOnVolume() will fail for a new root path alias for a volume.  E.g.,
** if the same net resource is connected to both X: and Y:, MyIsPathOnVolume()
** will only return TRUE for the drive root path that the net resource was
** connected to through the given HVOLUME.
*/
PUBLIC_CODE BOOL MyIsPathOnVolume(LPCTSTR pcszPath, HPATH hpath)
{
   BOOL bResult;
   TCHAR rgchVolumeRootPath[MAX_PATH_LEN];

   ASSERT(IsFullPath(pcszPath));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   if (IsVolumeAvailable(((PPATH)hpath)->hvol))
   {
      GetVolumeRootPath(((PPATH)hpath)->hvol, rgchVolumeRootPath);

      bResult = (MyLStrCmpNI(pcszPath, rgchVolumeRootPath,
                             lstrlen(rgchVolumeRootPath))
                 == CR_EQUAL);
   }
   else
   {
      TRACE_OUT((TEXT("MyIsPathOnVolume(): Failing on unavailable volume %s."),
                 DebugGetVolumeRootPath(((PPATH)hpath)->hvol, rgchVolumeRootPath)));

      bResult = FALSE;
   }

   return(bResult);
}


/*
** ComparePaths()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** PATHs are compared by:
**    1) volume
**    2) path suffix
*/
PUBLIC_CODE COMPARISONRESULT ComparePaths(HPATH hpath1, HPATH hpath2)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));

   /* This comparison works across path lists. */

   cr = ComparePathVolumes(hpath1, hpath2);

   if (cr == CR_EQUAL)
      cr = ComparePathStringsByHandle(((PCPATH)hpath1)->hsPathSuffix,
                                      ((PCPATH)hpath2)->hsPathSuffix);

   return(cr);
}


/*
** ComparePathVolumes()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT ComparePathVolumes(HPATH hpath1, HPATH hpath2)
{
   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));

   return(CompareVolumes(((PCPATH)hpath1)->hvol, ((PCPATH)hpath2)->hvol));
}


/*
** IsPathPrefix()
**
** Determines whether or not one path is a prefix of another.
**
** Arguments:     hpathChild - whole path (longer or same length)
**                hpathParent - prefix path to test (shorter or same length)
**
** Returns:       TRUE if the second path is a prefix of the first path.  FALSE
**                if not.
**
** Side Effects:  none
**
** Read 'IsPathPrefix(A, B)' as 'Is A in B's subtree?'.
*/
PUBLIC_CODE BOOL IsPathPrefix(HPATH hpathChild, HPATH hpathParent)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hpathParent, PATH));
   ASSERT(IS_VALID_HANDLE(hpathChild, PATH));

   if (ComparePathVolumes(hpathParent, hpathChild) == CR_EQUAL)
   {
      TCHAR rgchParentSuffix[MAX_PATH_LEN];
      TCHAR rgchChildSuffix[MAX_PATH_LEN];
      int nParentSuffixLen;
      int nChildSuffixLen;

      /* Ignore path roots when comparing path strings. */

      GetPathSuffixString(hpathParent, rgchParentSuffix);
      GetPathSuffixString(hpathChild, rgchChildSuffix);

      /* Only root paths should have no path suffix off the root. */

      nParentSuffixLen = lstrlen(rgchParentSuffix);
      nChildSuffixLen = lstrlen(rgchChildSuffix);

      /*
       * The parent path is a path prefix of the child path iff:
       *    1) The parent's path suffix string is shorter than or the same
       *       length as the child's path suffix string.
       *    2) The two path suffix strings match through the length of the
       *       parent's path suffix string.
       *    3) The prefix of the child's path suffix string is followed
       *       immediately by a null terminator or a path separator.
       */

      bResult = (nChildSuffixLen >= nParentSuffixLen &&
                 MyLStrCmpNI(rgchParentSuffix, rgchChildSuffix,
                             nParentSuffixLen) == CR_EQUAL &&
                 (nChildSuffixLen == nParentSuffixLen ||          /* same paths */
                  ! nParentSuffixLen ||                           /* root parent */
                  IS_SLASH(rgchChildSuffix[nParentSuffixLen])));  /* non-root parent */
   }
   else
      bResult = FALSE;

   return(bResult);
}


/*
** SubtreesIntersect()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** N.b., two subtrees cannot both intersect a third subtree unless they
** intersect each other.
*/
PUBLIC_CODE BOOL SubtreesIntersect(HPATH hpath1, HPATH hpath2)
{
   ASSERT(IS_VALID_HANDLE(hpath1, PATH));
   ASSERT(IS_VALID_HANDLE(hpath2, PATH));

   return(IsPathPrefix(hpath1, hpath2) ||
          IsPathPrefix(hpath2, hpath1));
}


/*
** FindEndOfRootSpec()
**
** Finds the end of the root specification in a path string.
**
** Arguments:     pcszPath - path to examine for root specification
**                hpath - handle to PATH that path string was generated from
**
** Returns:       pointer to first character after end of root specification
**
** Side Effects:  none
**
** Examples:
**
**    input path                    output string
**    ----------                    -------------
**    c:\                           <empty string>
**    c:\foo                        foo
**    c:\foo\bar                    foo\bar
**    \\pyrex\user\                 <empty string>
**    \\pyrex\user\foo              foo
**    \\pyrex\user\foo\bar          foo\bar
*/
PUBLIC_CODE LPTSTR FindEndOfRootSpec(LPCTSTR pcszFullPath, HPATH hpath)
{
   LPCTSTR pcsz;
   UINT ucchPathLen;
   UINT ucchSuffixLen;

   ASSERT(IsCanonicalPath(pcszFullPath));
   ASSERT(IS_VALID_HANDLE(hpath, PATH));

   ucchPathLen = lstrlen(pcszFullPath);
   ucchSuffixLen = lstrlen(GetString(((PCPATH)hpath)->hsPathSuffix));

   pcsz = pcszFullPath + ucchPathLen;

   if (ucchPathLen > ucchSuffixLen)
      pcsz -= ucchSuffixLen;
   else
      /* Assume path is root path. */
      ERROR_OUT((TEXT("FindEndOfRootSpec(): Path suffix %s is longer than full path %s."),
                 GetString(((PCPATH)hpath)->hsPathSuffix),
                 pcszFullPath));

   ASSERT(IsValidPathSuffix(pcsz));

   return((LPTSTR)pcsz);
}


/*
** FindPathSuffix()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE LPTSTR FindChildPathSuffix(HPATH hpathParent, HPATH hpathChild,
                                     LPTSTR pszChildSuffixBuf)
{
   LPCTSTR pcszChildSuffix;
   TCHAR rgchParentSuffix[MAX_PATH_LEN];

   ASSERT(IS_VALID_HANDLE(hpathParent, PATH));
   ASSERT(IS_VALID_HANDLE(hpathChild, PATH));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszChildSuffixBuf, STR, MAX_PATH_LEN));

   ASSERT(IsPathPrefix(hpathChild, hpathParent));

   GetPathSuffixString(hpathParent, rgchParentSuffix);
   GetPathSuffixString(hpathChild, pszChildSuffixBuf);

   ASSERT(lstrlen(rgchParentSuffix) <= lstrlen(pszChildSuffixBuf));
   pcszChildSuffix = pszChildSuffixBuf + lstrlen(rgchParentSuffix);

   if (IS_SLASH(*pcszChildSuffix))
      pcszChildSuffix++;

   ASSERT(IsValidPathSuffix(pcszChildSuffix));

   return((LPTSTR)pcszChildSuffix);
}


/*
** ComparePointers()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT ComparePointers(PCVOID pcv1, PCVOID pcv2)
{
   COMPARISONRESULT cr;

   /* pcv1 and pcv2 may be any value. */

   if (pcv1 < pcv2)
      cr = CR_FIRST_SMALLER;
   else if (pcv1 > pcv2)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   return(cr);
}


/*
** TWINRESULTFromLastError()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT TWINRESULTFromLastError(TWINRESULT tr)
{
   switch (GetLastError())
   {
      case ERROR_OUTOFMEMORY:
         tr = TR_OUT_OF_MEMORY;
         break;

      default:
         break;
   }

   return(tr);
}


/*
** WritePathList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT WritePathList(HCACHEDFILE hcf, HPATHLIST hpl)
{
   TWINRESULT tr;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));

   tr = WriteVolumeList(hcf, ((PCPATHLIST)hpl)->hvl);

   if (tr == TR_SUCCESS)
   {
      tr = WriteStringTable(hcf, ((PCPATHLIST)hpl)->hst);

      if (tr == TR_SUCCESS)
      {
         DWORD dwcbDBPathListHeaderOffset;

         tr = TR_BRIEFCASE_WRITE_FAILED;

         /* Save initial file position. */

         dwcbDBPathListHeaderOffset = GetCachedFilePointerPosition(hcf);

         if (dwcbDBPathListHeaderOffset != INVALID_SEEK_POSITION)
         {
            DBPATHLISTHEADER dbplh;

            /* Leave space for path list header. */

            ZeroMemory(&dbplh, sizeof(dbplh));

            if (WriteToCachedFile(hcf, (PCVOID)&dbplh, sizeof(dbplh), NULL))
            {
               ARRAYINDEX aicPtrs;
               ARRAYINDEX ai;
               LONG lcPaths = 0;

               tr = TR_SUCCESS;

               aicPtrs = GetPtrCount(((PCPATHLIST)hpl)->hpa);

               /* Write all paths. */

               for (ai = 0; ai < aicPtrs; ai++)
               {
                  PPATH ppath;

                  ppath = GetPtr(((PCPATHLIST)hpl)->hpa, ai);

                  /*
                   * As a sanity check, don't save any path with a lock count
                   * of 0.  A 0 lock count implies that the path has not been
                   * referenced since it was restored from the database, or
                   * something is broken.
                   */

                  if (ppath->ulcLock > 0)
                  {
                     tr = WritePath(hcf, ppath);

                     if (tr == TR_SUCCESS)
                     {
                        ASSERT(lcPaths < LONG_MAX);
                        lcPaths++;
                     }
                     else
                        break;
                  }
                  else
                     ERROR_OUT((TEXT("WritePathList(): PATH for path %s has 0 lock count and will not be written."),
                                DebugGetPathString((HPATH)ppath)));
               }

               /* Save path list header. */

               if (tr == TR_SUCCESS)
               {
                  dbplh.lcPaths = lcPaths;

                  tr = WriteDBSegmentHeader(hcf, dwcbDBPathListHeaderOffset, &dbplh,
                                            sizeof(dbplh));

                  TRACE_OUT((TEXT("WritePathList(): Wrote %ld paths."),
                             dbplh.lcPaths));
               }
            }
         }
      }
   }

   return(tr);
}


/*
** ReadPathList()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ReadPathList(HCACHEDFILE hcf, HPATHLIST hpl,
                                    PHHANDLETRANS phht)
{
   TWINRESULT tr;
   HHANDLETRANS hhtVolumes;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
   ASSERT(IS_VALID_WRITE_PTR(phht, HHANDLETRANS));

   tr = ReadVolumeList(hcf, ((PCPATHLIST)hpl)->hvl, &hhtVolumes);

   if (tr == TR_SUCCESS)
   {
      HHANDLETRANS hhtStrings;

      tr = ReadStringTable(hcf, ((PCPATHLIST)hpl)->hst, &hhtStrings);

      if (tr == TR_SUCCESS)
      {
         DBPATHLISTHEADER dbplh;
         DWORD dwcbRead;

         tr = TR_CORRUPT_BRIEFCASE;

         if (ReadFromCachedFile(hcf, &dbplh, sizeof(dbplh), &dwcbRead) &&
             dwcbRead == sizeof(dbplh))
         {
            HHANDLETRANS hht;

            if (CreateHandleTranslator(dbplh.lcPaths, &hht))
            {
               LONG l;

               tr = TR_SUCCESS;

               TRACE_OUT((TEXT("ReadPathList(): Reading %ld paths."),
                          dbplh.lcPaths));

               for (l = 0; l < dbplh.lcPaths; l++)
               {
                  tr = ReadPath(hcf, (PPATHLIST)hpl, hhtVolumes, hhtStrings,
                                hht);

                  if (tr != TR_SUCCESS)
                     break;
               }

               if (tr == TR_SUCCESS)
               {
                  PrepareForHandleTranslation(hht);
                  *phht = hht;

                  ASSERT(IS_VALID_HANDLE(hpl, PATHLIST));
                  ASSERT(IS_VALID_HANDLE(*phht, HANDLETRANS));
               }
               else
                  DestroyHandleTranslator(hht);
            }
            else
               tr = TR_OUT_OF_MEMORY;
         }

         DestroyHandleTranslator(hhtStrings);
      }

      DestroyHandleTranslator(hhtVolumes);
   }

   return(tr);
}


/*
** IsValidHPATH()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHPATH(HPATH hp)
{
   return(IS_VALID_STRUCT_PTR((PCPATH)hp, CPATH));
}


/*
** IsValidHVOLUMEID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHVOLUMEID(HVOLUMEID hvid)
{
   return(IS_VALID_HANDLE((HPATH)hvid, PATH));
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidHPATHLIST()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHPATHLIST(HPATHLIST hpl)
{
   return(IS_VALID_STRUCT_PTR((PCPATHLIST)hpl, CPATHLIST));
}

#endif


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | IsPathOnVolume | Determines whether or not a given path is on
a given volume.

@parm PCSTR | pcszPath | A pointer to a string indicating the path to be
checked.

@parm HVOLUMEID | hvid | A handle to a volume ID.

@parm PBOOL | pbOnVolume | A pointer to a BOOL to be filled in with TRUE if the
given path is on the given volume, or FALSE if not.  *pbOnVolume is only valid
if TR_SUCCESS is returned.

@rdesc If the volume check was successful, TR_SUCCESS is returned.  Otherwise,
the volume check was not successful, and the return value indicates the error
that occurred.

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI IsPathOnVolume(LPCTSTR pcszPath, HVOLUMEID hvid,
                                            PBOOL pbOnVolume)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(IsPathOnVolume);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_STRING_PTR(pcszPath, CSTR) &&
          IS_VALID_HANDLE(hvid, VOLUMEID) &&
          IS_VALID_WRITE_PTR(pbOnVolume, BOOL))
#endif
      {
         TCHAR rgchFullPath[MAX_PATH_LEN];
         LPTSTR pszFileName;
         DWORD dwPathLen;

         dwPathLen = GetFullPathName(pcszPath, ARRAYSIZE(rgchFullPath),
                                     rgchFullPath, &pszFileName);

         if (dwPathLen > 0 && dwPathLen < ARRAYSIZE(rgchFullPath))
         {
            *pbOnVolume = MyIsPathOnVolume(rgchFullPath, (HPATH)hvid);

            tr = TR_SUCCESS;
         }
         else
         {
            ASSERT(! dwPathLen);

            tr = TR_INVALID_PARAMETER;
         }
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(IsPathOnVolume, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}


/******************************************************************************

@doc SYNCENGAPI

@api TWINRESULT | GetVolumeDescription | Retrieves some descriptive information
for a volume, if that information is available.

@parm HVOLUMEID | hvid | A handle to a volume ID.

@parm PVOLUMEDESC | pvoldesc | A pointer to a VOLUMEDESC to be filled in with
information describing the volume.  The ulSize field of the VOLUMEDESC
structure should be filled in with sizeof(VOLUMEDESC) before calling
GetVolumeDescription().

@rdesc If the volume was described successfully, TR_SUCCESS is returned.
Otherwise, the volume was not described successfully, and the return value
indicates the error that occurred.

******************************************************************************/

SYNCENGAPI TWINRESULT WINAPI GetVolumeDescription(HVOLUMEID hvid,
                                                  PVOLUMEDESC pvoldesc)
{
   TWINRESULT tr;

   if (BeginExclusiveBriefcaseAccess())
   {
      DebugEntry(GetVolumeDescription);

#ifdef EXPV
      /* Verify parameters. */

      if (IS_VALID_HANDLE(hvid, VOLUMEID) &&
          IS_VALID_WRITE_PTR(pvoldesc, VOLUMEDESC) &&
          EVAL(pvoldesc->ulSize == sizeof(*pvoldesc)))
#endif
      {
         DescribeVolume(((PCPATH)hvid)->hvol, pvoldesc);

         tr = TR_SUCCESS;
      }
#ifdef EXPV
      else
         tr = TR_INVALID_PARAMETER;
#endif

      DebugExitTWINRESULT(GetVolumeDescription, tr);

      EndExclusiveBriefcaseAccess();
   }
   else
      tr = TR_REENTERED;

   return(tr);
}

