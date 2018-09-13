/*
 * linkinfo.c - LinkInfo ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "volumeid.h"
#include "cnrlink.h"


/* Macros
 *********/

/* macros for accessing ILINKINFO data */

#define ILI_Volume_ID_Ptr(pili) \
   ((PVOLUMEID)(((PBYTE)(pili)) + (pili)->ucbVolumeIDOffset))

#define ILI_Local_Base_Path_PtrA(pili) \
   ((LPSTR)(((PBYTE)(pili)) + (pili)->ucbLocalBasePathOffset))

#define ILI_CNR_Link_Ptr(pili) \
   ((PCNRLINK)(((PBYTE)(pili)) + (pili)->ucbCNRLinkOffset))

#define ILI_Common_Path_Suffix_PtrA(pili) \
   ((LPSTR)(((PBYTE)(pili)) + (pili)->ucbCommonPathSuffixOffset))

#define ILI_Local_Base_Path_PtrW(pili) \
   ((LPWSTR)(((PBYTE)(pili)) + (pili)->ucbLocalBasePathOffsetW))

#define ILI_Common_Path_Suffix_PtrW(pili) \
   ((LPWSTR)(((PBYTE)(pili)) + (pili)->ucbCommonPathSuffixOffsetW))

#ifdef UNICODE
#define ILI_Local_Base_Path_Ptr(pili)       ILI_Local_Base_Path_PtrW(pili)
#define ILI_Common_Path_Suffix_Ptr(pili)    ILI_Common_Path_Suffix_PtrW(pili)
#else
#define ILI_Local_Base_Path_Ptr(pili)       ILI_Local_Base_Path_PtrA(pili)
#define ILI_Common_Path_Suffix_Ptr(pili)    ILI_Common_Path_Suffix_PtrA(pili)
#endif

/* Types
 ********/

/******************************************************************************

@doc LINKINFOAPI

@struct LINKINFO | External definition of LinkInfo structure.

@field UINT | ucbSize | The size of the LINKINFO structure in bytes, including
the ucbSize field.  An ILINKINFO structure consists of a header described as
below, followed by variable-length data that is opaque to the caller.

******************************************************************************/

/*
   @doc INTERNAL

   @enum ILINKINFOFLAGS | Internal LinkInfo structure flags.
*/

typedef enum _ilinkinfoflags
{
   /*
      @emem ILI_FL_LOCAL_INFO_VALID | If set, volume ID and local path are
      valid.  If clear, volume ID and local path are not valid.
   */

   ILI_FL_LOCAL_INFO_VALID    =  0x0001,

   /*
      @emem ILI_FL_REMOTE_INFO_VALID | If set, CNRLink and path suffix are
      valid.  If clear, CNRLink and path suffix not valid.
   */

   ILI_FL_REMOTE_INFO_VALID   =  0x0002,

   /* @emem ALL_ILINKINFO_FLAGS | All internal LinkInfo structure flags. */

   ALL_ILINKINFO_FLAGS        = (ILI_FL_LOCAL_INFO_VALID |\
                                 ILI_FL_REMOTE_INFO_VALID)
}
ILINKINFOFLAGS;

/*
   @doc INTERNAL

   @struct ILINKINFO | Internal definition of relocatable, extensible, internal
   LinkInfo structure.  An ILINKINFO structure may contain an <t IVOLUMEID>
   structure and an <t ICNRLINK> structure.  An ILINKINFO structure consists of
   a header described as below, followed by variable-length data.
*/

typedef struct _ilinkinfoA
{
   /* @field LINKINFO | li | External <t LINKINFO> sub-structure. */

   LINKINFO li;

   /*
      @field UINT | ucbHeaderSize | Size of the ILINKINFO header structure in
      bytes.
   */

   UINT ucbHeaderSize;

   /*
      @field DWORD | dwFlags | A bit mask of flags from the <t ILINKINFOFLAGS>
      enumeration.
   */

   DWORD dwFlags;

   /*
      @field UINT | ucbVolumeIDOffset | Offset in bytes of <t IVOLUMEID>
      sub-structure from base of structure.
   */

   UINT ucbVolumeIDOffset;

   /*
      @field UINT | ucbLocalBasePathOffset | Offset in bytes of local base path
      string from base of structure.  The local base path is a valid file
      system path.  The local base path string + the common path suffix string
      form the local path string, which is a valid file system path.  The local
      base path string refers to the same resource as the CNRLink's CNR name
      string.<nl>

      Example local base path string: "c:\\work".<nl>
      E.g., if local path "c:\\work" is shared as "\\\\fredbird\\work", an
      ILinkInfo structure would break local path
      "c:\\work\\footwear\\sneakers.doc" up into local base path "c:\\work",
      CNRLink CNR name "\\\\fredbird\\work", and common path suffix
      "footwear\\sneakers.doc".
   */

   UINT ucbLocalBasePathOffset;

   /*
      @field UINT | ucbCNRLinkOffset | Offset in bytes of <t CNRLINK>
      sub-structure from base of structure.  The file system name of the
      CNRLink's CNR name + the common path suffix string form the remote path
      string, which is a valid file system path.  The CNRLink's CNR name string
      refers to the same resource as the local base path string.
   */

   UINT ucbCNRLinkOffset;

   /*
      @field UINT | ucbCommonPathSuffixOffset | Offset in bytes of common path
      suffix string from base of structure.<nl> Example common path suffix
      string: "footwear\\sneakers.doc".
   */

   UINT ucbCommonPathSuffixOffset;
}
ILINKINFOA;
DECLARE_STANDARD_TYPES(ILINKINFOA);

#ifdef UNICODE
typedef struct _ilinkinfoW
{
   /* @field LINKINFO | li | External <t LINKINFO> sub-structure. */

   LINKINFO li;

   /*
      @field UINT | ucbHeaderSize | Size of the ILINKINFO header structure in
      bytes.
   */

   UINT ucbHeaderSize;

   /*
      @field DWORD | dwFlags | A bit mask of flags from the <t ILINKINFOFLAGS>
      enumeration.
   */

   DWORD dwFlags;

   /*
      @field UINT | ucbVolumeIDOffset | Offset in bytes of <t IVOLUMEID>
      sub-structure from base of structure.
   */

   UINT ucbVolumeIDOffset;

   /*
      @field UINT | ucbLocalBasePathOffset | Offset in bytes of local base path
      string from base of structure.  The local base path is a valid file
      system path.  The local base path string + the common path suffix string
      form the local path string, which is a valid file system path.  The local
      base path string refers to the same resource as the CNRLink's CNR name
      string.<nl>

      Example local base path string: "c:\\work".<nl>
      E.g., if local path "c:\\work" is shared as "\\\\fredbird\\work", an
      ILinkInfo structure would break local path
      "c:\\work\\footwear\\sneakers.doc" up into local base path "c:\\work",
      CNRLink CNR name "\\\\fredbird\\work", and common path suffix
      "footwear\\sneakers.doc".
   */

   UINT ucbLocalBasePathOffset;

   /*
      @field UINT | ucbCNRLinkOffset | Offset in bytes of <t CNRLINK>
      sub-structure from base of structure.  The file system name of the
      CNRLink's CNR name + the common path suffix string form the remote path
      string, which is a valid file system path.  The CNRLink's CNR name string
      refers to the same resource as the local base path string.
   */

   UINT ucbCNRLinkOffset;

   /*
      @field UINT | ucbCommonPathSuffixOffset | Offset in bytes of common path
      suffix string from base of structure.<nl> Example common path suffix
      string: "footwear\\sneakers.doc".
   */

   UINT ucbCommonPathSuffixOffset;

   /*
     These fields duplicate the above ones except that they are for the unicode
     versions of the strings.
   */
   UINT ucbLocalBasePathOffsetW;
   UINT ucbCommonPathSuffixOffsetW;

}
ILINKINFOW;
DECLARE_STANDARD_TYPES(ILINKINFOW);

#endif

#ifdef UNICODE
#define ILINKINFO   ILINKINFOW
#define PILINKINFO  PILINKINFOW
#define CILINKINFO  CILINKINFOW
#define PCILINKINFO PCILINKINFOW
#else
#define ILINKINFO   ILINKINFOA
#define PILINKINFO  PILINKINFOA
#define CILINKINFO  CILINKINFOA
#define PCILINKINFO PCILINKINFOA
#endif


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL CreateILinkInfo(LPCTSTR, PILINKINFO *);
PRIVATE_CODE BOOL CreateLocalILinkInfo(LPCTSTR, PILINKINFO *);
PRIVATE_CODE BOOL CreateRemoteILinkInfo(LPCTSTR, LPCTSTR, LPCTSTR, PILINKINFO *);
PRIVATE_CODE BOOL UnifyILinkInfo(PCVOLUMEID, UINT, LPCTSTR, PCCNRLINK, UINT, LPCTSTR, PILINKINFO *);
PRIVATE_CODE void DestroyILinkInfo(PILINKINFO);
PRIVATE_CODE BOOL UpdateILinkInfo(PCILINKINFO, LPCTSTR, PDWORD, PILINKINFO *);
PRIVATE_CODE BOOL UseNewILinkInfo(PCILINKINFO, PCILINKINFO);
PRIVATE_CODE BOOL ResolveLocalILinkInfo(PCILINKINFO, LPTSTR, DWORD);
PRIVATE_CODE BOOL ResolveRemoteILinkInfo(PCILINKINFO, LPTSTR, DWORD, HWND, PDWORD);
PRIVATE_CODE BOOL ResolveILinkInfo(PCILINKINFO, LPTSTR, DWORD, HWND, PDWORD);
PRIVATE_CODE BOOL ResolveLocalPathFromServer(PCILINKINFO, LPTSTR, PDWORD);
PRIVATE_CODE void GetLocalPathFromILinkInfo(PCILINKINFO, LPTSTR);
PRIVATE_CODE void GetRemotePathFromILinkInfo(PCILINKINFO, LPTSTR);
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoReferents(PCILINKINFO, PCILINKINFO);
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoLocalData(PCILINKINFO, PCILINKINFO);
PRIVATE_CODE COMPARISONRESULT CompareLocalPaths(PCILINKINFO, PCILINKINFO);
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoRemoteData(PCILINKINFO, PCILINKINFO);
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoVolumes(PCILINKINFO, PCILINKINFO);
PRIVATE_CODE BOOL CheckCombinedPathLen(LPCTSTR, LPCTSTR);
PRIVATE_CODE BOOL GetILinkInfoData(PCILINKINFO, LINKINFODATATYPE, PCVOID *);
PRIVATE_CODE BOOL DisconnectILinkInfo(PCILINKINFO);

#if defined(DEBUG) || defined(EXPV)

PRIVATE_CODE BOOL IsValidLINKINFODATATYPE(LINKINFODATATYPE);

#endif

#if defined(DEBUG) || defined(VSTF)

PRIVATE_CODE BOOL CheckILIFlags(PCILINKINFO);
PRIVATE_CODE BOOL CheckILICommonPathSuffix(PCILINKINFO);
PRIVATE_CODE BOOL CheckILILocalInfo(PCILINKINFO);
PRIVATE_CODE BOOL CheckILIRemoteInfo(PCILINKINFO);
PRIVATE_CODE BOOL IsValidPCLINKINFO(PCLINKINFO);
PRIVATE_CODE BOOL IsValidPCILINKINFO(PCILINKINFO);

#ifdef SKIP_OLD_LINKINFO_QUIETLY
PRIVATE_CODE BOOL IsNewLinkInfoHackCheck(PCILINKINFO);
#endif

#endif

#ifdef DEBUG

PRIVATE_CODE void DumpILinkInfo(PCILINKINFO);

#endif


/*
** CreateILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateILinkInfo(LPCTSTR pcszPath, PILINKINFO *ppili)
{
   BOOL bResult = FALSE;
   TCHAR rgchCanonicalPath[MAX_PATH_LEN];
   DWORD dwCanonicalPathFlags;
   TCHAR rgchCNRName[MAX_PATH_LEN];
   LPTSTR pszRootPathSuffix;

   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppili, PILINKINFO));

   if (GetCanonicalPathInfo(pcszPath, rgchCanonicalPath, &dwCanonicalPathFlags,
                            rgchCNRName, &pszRootPathSuffix))
   {
      if (IS_FLAG_SET(dwCanonicalPathFlags, GCPI_OFL_REMOTE))
         bResult = CreateRemoteILinkInfo(rgchCanonicalPath, rgchCNRName,
                                         pszRootPathSuffix, ppili);
      else
         bResult = CreateLocalILinkInfo(rgchCanonicalPath, ppili);
   }

   return(bResult);
}


/*
** CreateLocalILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateLocalILinkInfo(LPCTSTR pcszLocalPath, PILINKINFO *ppili)
{
   BOOL bResult;
   PVOLUMEID pvolid;
   UINT ucbVolumeIDLen;

   ASSERT(IsLocalDrivePath(pcszLocalPath));
   ASSERT(IS_VALID_WRITE_PTR(ppili, PILINKINFO));

   bResult = CreateVolumeID(pcszLocalPath, &pvolid, &ucbVolumeIDLen);

   if (bResult)
   {
      PCNRLINK pcnrl;
      UINT ucbCNRLinkLen;
      TCHAR rgchLocalBasePath[MAX_PATH_LEN];
      LPCTSTR pcszCommonPathSuffix;

      bResult = CreateLocalCNRLink(pcszLocalPath, &pcnrl, &ucbCNRLinkLen,
                                   rgchLocalBasePath, &pcszCommonPathSuffix);

      if (bResult)
      {
         /* Wrap them up. */

         bResult = UnifyILinkInfo(pvolid, ucbVolumeIDLen, rgchLocalBasePath,
                                  pcnrl, ucbCNRLinkLen, pcszCommonPathSuffix,
                                  ppili);

         if (ucbCNRLinkLen > 0)
            DestroyCNRLink(pcnrl);
      }

      if (ucbVolumeIDLen > 0)
         DestroyVolumeID(pvolid);
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppili, CILINKINFO));

   return(bResult);
}


/*
** CreateRemoteILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CreateRemoteILinkInfo(LPCTSTR pcszRemotePath,
                                        LPCTSTR pcszCNRName,
                                        LPCTSTR pcszRootPathSuffix,
                                        PILINKINFO *ppili)
{
   BOOL bResult;
   PCNRLINK pcnrl;
   UINT ucbCNRLinkLen;

   ASSERT(IsCanonicalPath(pcszRemotePath));
   ASSERT(IsValidCNRName(pcszCNRName));
   ASSERT(IS_VALID_STRING_PTR(pcszRootPathSuffix, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppili, PILINKINFO));

   bResult = CreateRemoteCNRLink(pcszRemotePath, pcszCNRName, &pcnrl,
                                 &ucbCNRLinkLen);

   if (bResult)
   {
      /* Wrap it up. */

      bResult = UnifyILinkInfo(NULL, 0, EMPTY_STRING, pcnrl, ucbCNRLinkLen,
                               pcszRootPathSuffix, ppili);

      if (EVAL(ucbCNRLinkLen > 0))
         DestroyCNRLink(pcnrl);
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppili, CILINKINFO));

   return(bResult);
}


/*
** UnifyILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnifyILinkInfo(PCVOLUMEID pcvolid, UINT ucbVolumeIDLen,
                            LPCTSTR pcszLocalBasePath, PCCNRLINK pccnrl,
                            UINT ucbCNRLinkLen, LPCTSTR pcszCommonPathSuffix,
                            PILINKINFO *ppili)
{
   BOOL bResult;
   UINT ucbILinkInfoLen;
   UINT ucbDataOffset;
   UINT cbAnsiLocalBasePath;
   UINT cbAnsiCommonPathSuffix;
#ifdef UNICODE
   BOOL bUnicode;
   UINT cchChars;
   CHAR szAnsiLocalBasePath[MAX_PATH*2];
   CHAR szAnsiCommonPathSuffix[MAX_PATH*2];
   UINT cbWideLocalBasePath;
   UINT cbWideCommonPathSuffix;
   UINT cbChars;
#endif

   ASSERT(! ucbVolumeIDLen ||
          (IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID) &&
           IsDrivePath(pcszLocalBasePath)));
   ASSERT(! ucbCNRLinkLen ||
          IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));
   ASSERT(IS_VALID_STRING_PTR(pcszCommonPathSuffix, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppili, PILINKINFO));

#ifdef UNICODE
   bUnicode = FALSE;

   /*
   ** Convert the common-path string from UNICODE->ansi and back again
   ** to determine if the string contains any non-ansi characters.  If no
   ** characters are lost in the conversion then the string contains only 
   ** ansi chars.
   */
   cbAnsiCommonPathSuffix = WideCharToMultiByte(CP_ACP, 0,
                                          pcszCommonPathSuffix, -1,
                                          szAnsiCommonPathSuffix, ARRAYSIZE(szAnsiCommonPathSuffix),
                                          0, 0);
   if ( cbAnsiCommonPathSuffix == 0 )
   {
      bUnicode = FALSE;
   }
   else
   {
      WCHAR szWideCommonPathSuffix[MAX_PATH];

      cbChars = MultiByteToWideChar(CP_ACP, 0,
                                    szAnsiCommonPathSuffix, -1,
                                    szWideCommonPathSuffix, MAX_PATH);
      if ( cbChars == 0 || lstrcmp(pcszCommonPathSuffix,szWideCommonPathSuffix) != 0 )
      {
         bUnicode = TRUE;
      }
   }

   if (ucbVolumeIDLen > 0)
   {
      /*
      ** Convert the localbase-path string from UNICODE->ansi and back again
      ** to determine if the string contains any non-ansi characters.  If no
      ** characters are lost in the conversion then the string contains only 
      ** ansi chars.
      */
      cbAnsiLocalBasePath = WideCharToMultiByte(CP_ACP, 0,
                                         pcszLocalBasePath, -1,
                                         szAnsiLocalBasePath, MAX_PATH*2,
                                         0, 0);
      if ( cbAnsiLocalBasePath == 0 )
      {
         bUnicode = FALSE;
      }
      else
      {
         WCHAR szWideLocalBasePath[MAX_PATH];

         cchChars = MultiByteToWideChar(CP_ACP, 0,
                                        szAnsiLocalBasePath, -1,
                                        szWideLocalBasePath, ARRAYSIZE(szWideLocalBasePath));
         if ( cchChars == 0 || lstrcmp(pcszLocalBasePath,szWideLocalBasePath) != 0 )
         {
            bUnicode = TRUE;
         }
      }
   }
   else
   {
      cbAnsiLocalBasePath = 0;
   }

   if ( bUnicode )
   {
      ucbDataOffset = SIZEOF(ILINKINFOW);

      /* (+ 1) for null terminator. */
      cbWideCommonPathSuffix = (lstrlen(pcszCommonPathSuffix) + 1) * sizeof(TCHAR);

      if (ucbVolumeIDLen > 0)
         cbWideLocalBasePath = (lstrlen(pcszLocalBasePath) + 1) * sizeof(TCHAR);
      else
         cbWideLocalBasePath = 0;

   }
   else
   {
      ucbDataOffset = SIZEOF(ILINKINFOA);

      cbWideCommonPathSuffix = 0;
      cbWideLocalBasePath  = 0;
   }

   ucbILinkInfoLen = ucbDataOffset +
                     ucbVolumeIDLen +
                     cbAnsiLocalBasePath;
   if ( bUnicode && ucbVolumeIDLen > 0 )
   {
      ucbILinkInfoLen = ALIGN_WORD_CNT(ucbILinkInfoLen);
      ucbILinkInfoLen += cbWideLocalBasePath;
   }
   if ( ucbCNRLinkLen > 0 )
   {
      ucbILinkInfoLen = ALIGN_DWORD_CNT(ucbILinkInfoLen);
      ucbILinkInfoLen += ucbCNRLinkLen;
   }
   ucbILinkInfoLen += cbAnsiCommonPathSuffix;
   if ( bUnicode )
   {
      ucbILinkInfoLen = ALIGN_WORD_CNT(ucbILinkInfoLen);
      ucbILinkInfoLen += cbWideCommonPathSuffix;
   }

#else

   /* Calculate total length. */

   /* Assume we don't overflow ucbILinkInfoLen here. */

   /*
    * Base structure size plus common path suffix length.  (+ 1) for null
    * terminator.
    */
   cbAnsiCommonPathSuffix = lstrlen(pcszCommonPathSuffix) + 1;

   ucbILinkInfoLen = SIZEOF(**ppili) +
                     cbAnsiCommonPathSuffix;

   /* Plus size of local information. */

   if (ucbVolumeIDLen > 0)
   {
      /* (+ 1) for null terminator. */
      cbAnsiLocalBasePath = lstrlen(pcszLocalBasePath) + 1;

      ucbILinkInfoLen += ucbVolumeIDLen +
                         cbAnsiLocalBasePath;
   }

   /* Plus size of remote information. */

   if (ucbCNRLinkLen > 0)
      /* (+ 1) for null terminator. */
      ucbILinkInfoLen += ucbCNRLinkLen;

   ucbDataOffset = SIZEOF(**ppili);
#endif

   /* Try to allocate a container. */

   bResult = AllocateMemory(ucbILinkInfoLen, ppili);

   if (bResult)
   {
      (*ppili)->li.ucbSize = ucbILinkInfoLen;

      (*ppili)->ucbHeaderSize = ucbDataOffset;
      (*ppili)->dwFlags = 0;

      /* Do we have local information? */

      if (ucbVolumeIDLen > 0)
      {
         /* Yes.  Add it to the structure. */

         ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));
         ASSERT(IsDrivePath(pcszLocalBasePath));

         /* Append local volume ID. */

         (*ppili)->ucbVolumeIDOffset = ucbDataOffset;
         CopyMemory(ILI_Volume_ID_Ptr(*ppili), pcvolid, ucbVolumeIDLen);
         ucbDataOffset += ucbVolumeIDLen;

         /* Append local path. */

         (*ppili)->ucbLocalBasePathOffset = ucbDataOffset;
#ifdef UNICODE
         lstrcpyA(ILI_Local_Base_Path_PtrA(*ppili), szAnsiLocalBasePath);
         ucbDataOffset += cbAnsiLocalBasePath;

         if ( bUnicode )
         {
            ucbDataOffset = ALIGN_WORD_CNT(ucbDataOffset);
            (*ppili)->ucbLocalBasePathOffsetW = ucbDataOffset;
            lstrcpy(ILI_Local_Base_Path_PtrW(*ppili), pcszLocalBasePath);
            ucbDataOffset += cbWideLocalBasePath;
         }
#else
         lstrcpy(ILI_Local_Base_Path_Ptr(*ppili), pcszLocalBasePath);
         ucbDataOffset += cbAnsiLocalBasePath;
#endif
         SET_FLAG((*ppili)->dwFlags, ILI_FL_LOCAL_INFO_VALID);
      }

      /* Do we have remote information? */

      if (ucbCNRLinkLen > 0)
      {
         ucbDataOffset = ALIGN_DWORD_CNT(ucbDataOffset);

         /* Yes.  Add it to the structure. */

         ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

         /* Append CNR link. */

         (*ppili)->ucbCNRLinkOffset = ucbDataOffset;
         CopyMemory(ILI_CNR_Link_Ptr(*ppili), pccnrl, ucbCNRLinkLen);
         ucbDataOffset += ucbCNRLinkLen;

         SET_FLAG((*ppili)->dwFlags, ILI_FL_REMOTE_INFO_VALID);
      }

      /* Append common path suffix. */

      ASSERT(IS_VALID_STRING_PTR(pcszCommonPathSuffix, CSTR));

      (*ppili)->ucbCommonPathSuffixOffset = ucbDataOffset;
#ifdef UNICODE
      lstrcpyA(ILI_Common_Path_Suffix_PtrA(*ppili), szAnsiCommonPathSuffix);
      ucbDataOffset += cbAnsiCommonPathSuffix;
      if ( bUnicode )
      {
         ucbDataOffset = ALIGN_WORD_CNT(ucbDataOffset);

         (*ppili)->ucbCommonPathSuffixOffsetW = ucbDataOffset;
         lstrcpy(ILI_Common_Path_Suffix_Ptr(*ppili), pcszCommonPathSuffix);
         ucbDataOffset += cbWideCommonPathSuffix;
      }
#else /* UNICODE */
      lstrcpy(ILI_Common_Path_Suffix_Ptr(*ppili), pcszCommonPathSuffix);
#ifdef DEBUG
      /*
      ** NOTE:  This same increment was present above in the UNICODE section
      **        enclosed in an #ifdef DEBUG block.
      **        It was causing the assertion below (ucbDataOffset == ucbILinkInfoLen)
      **        to fail.  I have left stmt instance in the ansi build untouched.
      **        If the assertion fails in the ansi build you should 
      **        try removing this next statement. [brianau - 4/15/99]
      */
      ucbDataOffset += cbAnsiCommonPathSuffix;
#endif
#endif

      /* Do all the calculated lengths match? */

      // ASSERT(ucbDataOffset == (*ppili)->li.ucbSize);
      ASSERT(ucbDataOffset == ucbILinkInfoLen);
   }

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppili, CILINKINFO));

   return(bResult);
}


/*
** DestroyILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DestroyILinkInfo(PILINKINFO pili)
{
   ASSERT(IS_VALID_STRUCT_PTR(pili, CILINKINFO));

   FreeMemory(pili);

   return;
}


/*
** UpdateILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** An ILinkInfo structure is updated in the following cases:
**
** local information:
**
**    1) the local path has changed
**    2) remote information is available for the local path
**
** remote information:
**
**    3) the remote information is local to this machine, and local information
**       is available for the remote path
*/
PRIVATE_CODE BOOL UpdateILinkInfo(PCILINKINFO pcili, LPCTSTR pcszResolvedPath,
                                  PDWORD pdwOutFlags, PILINKINFO *ppiliUpdated)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_STRING_PTR(pcszResolvedPath, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));
   ASSERT(IS_VALID_WRITE_PTR(ppiliUpdated, PILINKINFO));

   *pdwOutFlags = 0;

   bResult = CreateILinkInfo(pcszResolvedPath, ppiliUpdated);

   if (bResult)
   {
      if (UseNewILinkInfo(pcili, *ppiliUpdated))
      {
         SET_FLAG(*pdwOutFlags, RLI_OFL_UPDATED);

         WARNING_OUT((TEXT("UpdateILinkInfo(): Updating ILinkInfo for path %s."),
                      pcszResolvedPath));
      }
   }

   ASSERT(! bResult ||
          (IS_FLAG_CLEAR(*pdwOutFlags, RLI_OFL_UPDATED) ||
           IS_VALID_STRUCT_PTR(*ppiliUpdated, CILINKINFO)));

   return(bResult);
}


/*
** UseNewILinkInfo()
**
**
**
** Arguments:
**
** Returns:       TRUE if the new ILinkInfo structure contains more or
**                different information than the old ILinkInfo structure.
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UseNewILinkInfo(PCILINKINFO pciliOld, PCILINKINFO pciliNew)
{
   BOOL bUpdate = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pciliOld, CILINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pciliNew, CILINKINFO));

   /* Does the new ILinkInfo structure contain local information? */

   if (IS_FLAG_SET(pciliNew->dwFlags, ILI_FL_LOCAL_INFO_VALID))
   {
      /* Yes.  Does the old ILinkInfo structure contain local information? */

      if (IS_FLAG_SET(pciliOld->dwFlags, ILI_FL_LOCAL_INFO_VALID))
         /*
          * Yes.  Update the old ILinkInfo structure if local information
          * differs.
          */
         bUpdate = (CompareILinkInfoLocalData(pciliOld, pciliNew) != CR_EQUAL);
      else
         /* No.  Update the old ILinkInfo structure. */
         bUpdate = TRUE;
   }
   else
      /* No.  Do not update the old ILinkInfo structure. */
      bUpdate = FALSE;

   /*
    * Do we already need to update the old ILinkInfo structure based on local
    * information comparison?
    */

   if (! bUpdate)
   {
      /* No.  Compare remote information. */

      /* Does the new ILinkInfo structure contain remote information? */

      if (IS_FLAG_SET(pciliNew->dwFlags, ILI_FL_REMOTE_INFO_VALID))
      {
         /*
          * Yes.  Does the old ILinkInfo structure contain remote information?
          */

         if (IS_FLAG_SET(pciliOld->dwFlags, ILI_FL_REMOTE_INFO_VALID))
            /*
             * Yes.  Update the old ILinkInfo structure if remote information
             * differs.
             */
            bUpdate = (CompareILinkInfoRemoteData(pciliOld, pciliNew)
                       != CR_EQUAL);
         else
            /* No.  Update the old ILinkInfo structure. */
            bUpdate = TRUE;
      }
   }

   return(bUpdate);
}


/*
** ResolveLocalILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL ResolveLocalILinkInfo(PCILINKINFO pcili,
                                        LPTSTR pszResolvedPathBuf,
                                        DWORD dwInFlags)
{
   BOOL bResult;
   DWORD dwLocalSearchFlags;
   TCHAR rgchLocalPath[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_RLI_IFLAGS));

   /* Search for local path. */

   TRACE_OUT((TEXT("ResolveLocalILinkInfo(): Attempting to resolve LinkInfo locally.")));

   GetLocalPathFromILinkInfo(pcili, rgchLocalPath);

   if (IS_FLAG_SET(dwInFlags, RLI_IFL_LOCAL_SEARCH))
      dwLocalSearchFlags = SFLP_IFL_LOCAL_SEARCH;
   else
      dwLocalSearchFlags = 0;

   bResult = SearchForLocalPath(ILI_Volume_ID_Ptr(pcili), rgchLocalPath,
                                dwLocalSearchFlags, pszResolvedPathBuf);

   ASSERT(! bResult ||
          EVAL(IsCanonicalPath(pszResolvedPathBuf)));

   return(bResult);
}


/*
** ResolveRemoteILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL ResolveRemoteILinkInfo(PCILINKINFO pcili,
                                         LPTSTR pszResolvedPathBuf,
                                         DWORD dwInFlags, HWND hwndOwner,
                                         PDWORD pdwOutFlags)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_RLI_IFLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, RLI_IFL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));

   ASSERT(IS_FLAG_CLEAR(dwInFlags, RLI_IFL_TEMPORARY) ||
          IS_FLAG_SET(dwInFlags, RLI_IFL_CONNECT));

   TRACE_OUT((TEXT("ResolveRemoteILinkInfo(): Attempting to resolve LinkInfo remotely.")));

   /* Connect if requested. */

   if (IS_FLAG_SET(dwInFlags, RLI_IFL_CONNECT))
   {
      DWORD dwConnectInFlags;
      DWORD dwConnectOutFlags;

      dwConnectInFlags = 0;

      if (IS_FLAG_SET(dwInFlags, RLI_IFL_TEMPORARY))
         dwConnectInFlags = CONNECT_TEMPORARY;

      if (IS_FLAG_SET(dwInFlags, RLI_IFL_ALLOW_UI))
         SET_FLAG(dwConnectInFlags, CONNECT_INTERACTIVE);

      if (IS_FLAG_SET(dwInFlags, RLI_IFL_REDIRECT))
         SET_FLAG(dwConnectInFlags, CONNECT_REDIRECT);

      bResult = ConnectToCNR(ILI_CNR_Link_Ptr(pcili), dwConnectInFlags,
                             hwndOwner, pszResolvedPathBuf,
                             &dwConnectOutFlags);

      if (bResult)
      {
#ifdef UNICODE
         WCHAR szWideCommonPathSuffix[MAX_PATH];
         LPWSTR pszWideCommonPathSuffix;

         if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
         {
            pszWideCommonPathSuffix = szWideCommonPathSuffix;
            MultiByteToWideChar(CP_ACP, 0,
                                ILI_Common_Path_Suffix_PtrA(pcili), -1,
                                szWideCommonPathSuffix, MAX_PATH);
         }
         else
         {
            pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
         }
         CatPath(pszResolvedPathBuf, pszWideCommonPathSuffix);
#else
         CatPath(pszResolvedPathBuf, ILI_Common_Path_Suffix_Ptr(pcili));
#endif

         if (IS_FLAG_SET(dwConnectOutFlags, CONNECT_REFCOUNT))
         {
            ASSERT(IS_FLAG_CLEAR(dwConnectOutFlags, CONNECT_LOCALDRIVE));

            SET_FLAG(*pdwOutFlags, RLI_OFL_DISCONNECT);
         }
      }
   }
   else
   {
      /*
       * It's ok that IsCNRAvailable() and GetRemotePathFromILinkInfo() are
       * broken for NPs whose CNR names are not valid file system root paths.
       *
       * For NPs whose CNR names are valid file system root paths,
       * IsCNRAvailable() will succeed or fail, and
       * GetRemotePathFromILinkInfo() will be called only on success.
       *
       * For NPs whose CNR names are not valid file system root paths,
       * IsCNRAvailable() will fail and GetRemotePathFromILinkInfo() will not
       * be called.
       */

      bResult = IsCNRAvailable(ILI_CNR_Link_Ptr(pcili));

      if (bResult)
         GetRemotePathFromILinkInfo(pcili, pszResolvedPathBuf);
   }

   ASSERT(! bResult ||
          (EVAL(IsCanonicalPath(pszResolvedPathBuf)) &&
           FLAGS_ARE_VALID(*pdwOutFlags, ALL_RLI_OFLAGS)));

   return(bResult);
}


/*
** ResolveILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL ResolveILinkInfo(PCILINKINFO pcili, LPTSTR pszResolvedPathBuf,
                                   DWORD dwInFlags, HWND hwndOwner,
                                   PDWORD pdwOutFlags)
{
   BOOL bResult;
   BOOL bLocalInfoValid;
   BOOL bRemoteInfoValid;
   BOOL bLocalShare;

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_RLI_IFLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, RLI_IFL_ALLOW_UI) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));

   *pdwOutFlags = 0;

   /* Describe LinkInfo contents. */

   bRemoteInfoValid = IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID);
   bLocalInfoValid = IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID);

   ASSERT(bLocalInfoValid || bRemoteInfoValid);

   /*
    * RAIDRAID: (15703) We will resolve to the wrong local path for a share
    * that has been moved to another path here.
    */

   bLocalShare = FALSE;

   if (bRemoteInfoValid)
   {
      DWORD dwLocalShareFlags;

      /* Ask the server for the local path. */

      bResult = ResolveLocalPathFromServer(pcili, pszResolvedPathBuf,
                                           &dwLocalShareFlags);

      if (IS_FLAG_SET(dwLocalShareFlags, CNR_FL_LOCAL))
         bLocalShare = TRUE;

      if (bResult)
      {
         ASSERT(IS_FLAG_SET(dwLocalShareFlags, CNR_FL_LOCAL));

         TRACE_OUT((TEXT("ResolveILinkInfo(): Resolved local path from server.")));
      }
   }
   else
      /* Can't tell if the referent is local or not. */
      bResult = FALSE;

   if (! bResult)
   {
      /* Try local path. */

      if (bLocalInfoValid)
         bResult = ResolveLocalILinkInfo(pcili, pszResolvedPathBuf, dwInFlags);

      if (! bResult)
      {
         /* Try remote path. */

         if (bRemoteInfoValid && ! bLocalShare)
            bResult = ResolveRemoteILinkInfo(pcili, pszResolvedPathBuf,
                                             dwInFlags, hwndOwner,
                                             pdwOutFlags);
      }
   }

   ASSERT(! bResult ||
          (EVAL(IsCanonicalPath(pszResolvedPathBuf)) &&
           FLAGS_ARE_VALID(*pdwOutFlags, ALL_RLI_OFLAGS)));

   return(bResult);
}


/*
** ResolveLocalPathFromServer()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL ResolveLocalPathFromServer(PCILINKINFO pcili,
                                             LPTSTR pszResolvedPathBuf,
                                             PDWORD pdwOutFlags)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));

   ASSERT(IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID));

   /* Try to get local path from server. */

   bResult = GetLocalPathFromCNRLink(ILI_CNR_Link_Ptr(pcili),
                                     pszResolvedPathBuf, pdwOutFlags);

   if (bResult)
   {

#ifdef UNICODE
      WCHAR szWideCommonPathSuffix[MAX_PATH];
      LPWSTR pszWideCommonPathSuffix;

      ASSERT(IS_FLAG_SET(*pdwOutFlags, CNR_FL_LOCAL));

      if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
      {
         pszWideCommonPathSuffix = szWideCommonPathSuffix;
         MultiByteToWideChar(CP_ACP, 0,
                             ILI_Common_Path_Suffix_PtrA(pcili), -1,
                             szWideCommonPathSuffix, MAX_PATH);
      }
      else
      {
         pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
      }
      CatPath(pszResolvedPathBuf, pszWideCommonPathSuffix);
#else
      ASSERT(IS_FLAG_SET(*pdwOutFlags, CNR_FL_LOCAL));

      CatPath(pszResolvedPathBuf, ILI_Common_Path_Suffix_Ptr(pcili));
#endif
   }

   ASSERT(FLAGS_ARE_VALID(*pdwOutFlags, ALL_CNR_FLAGS) &&
          (! bResult ||
           (EVAL(IS_FLAG_SET(*pdwOutFlags, CNR_FL_LOCAL)) &&
            EVAL(IsLocalDrivePath(pszResolvedPathBuf)))));

   return(bResult);
}


/*
** GetLocalPathFromILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void GetLocalPathFromILinkInfo(PCILINKINFO pcili,
                                       LPTSTR pszResolvedPathBuf)
{
#ifdef UNICODE
   WCHAR szWideLocalBasePath[MAX_PATH];
   LPWSTR pszWideLocalBasePath;
   WCHAR szWideCommonPathSuffix[MAX_PATH];
   LPWSTR pszWideCommonPathSuffix;
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN));

#ifdef UNICODE

   if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
   {
      pszWideLocalBasePath = szWideLocalBasePath;
      MultiByteToWideChar(CP_ACP, 0,
                          ILI_Local_Base_Path_PtrA(pcili), -1,
                          szWideLocalBasePath, MAX_PATH);

      pszWideCommonPathSuffix = szWideCommonPathSuffix;
      MultiByteToWideChar(CP_ACP, 0,
                          ILI_Common_Path_Suffix_PtrA(pcili), -1,
                          szWideCommonPathSuffix, MAX_PATH);
   }
   else
   {
      pszWideLocalBasePath    = ILI_Local_Base_Path_Ptr(pcili);
      pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
   }
   lstrcpy(pszResolvedPathBuf, pszWideLocalBasePath);
   CatPath(pszResolvedPathBuf, pszWideCommonPathSuffix);
#else
   lstrcpy(pszResolvedPathBuf, ILI_Local_Base_Path_Ptr(pcili));
   CatPath(pszResolvedPathBuf, ILI_Common_Path_Suffix_Ptr(pcili));
#endif

   ASSERT(lstrlen(pszResolvedPathBuf) < MAX_PATH_LEN);
   ASSERT(IsDrivePath(pszResolvedPathBuf));

   return;
}


/*
** GetRemotePathFromILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void GetRemotePathFromILinkInfo(PCILINKINFO pcili,
                                        LPTSTR pszResolvedPathBuf)
{
#ifdef UNICODE
   WCHAR szWideCommonPathSuffix[MAX_PATH];
   LPWSTR pszWideCommonPathSuffix;
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN));

   /* It's ok that this is broken for non-UNC CNR names. */

   GetRemotePathFromCNRLink(ILI_CNR_Link_Ptr(pcili), pszResolvedPathBuf);

#ifdef UNICODE
   if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
   {
      pszWideCommonPathSuffix = szWideCommonPathSuffix;
      MultiByteToWideChar(CP_ACP, 0,
                          ILI_Common_Path_Suffix_PtrA(pcili), -1,
                          szWideCommonPathSuffix, MAX_PATH);
   }
   else
   {
      pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
   }
   CatPath(pszResolvedPathBuf, pszWideCommonPathSuffix);
#else
   CatPath(pszResolvedPathBuf, ILI_Common_Path_Suffix_Ptr(pcili));
#endif

   return;
}


/*
** CompareILinkInfoReferents()
**
** Compares the referents of two ILINKINFO structures.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Comparison is performed on ILINKINFO data in only one of the following ways
** in the following order:
**
**    1) local data compared with local data
**    2) remote data compared with remote data
**    3) local data only < remote data only
*/
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoReferents(PCILINKINFO pciliFirst,
                                                   PCILINKINFO pciliSecond)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pciliFirst, CILINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pciliSecond, CILINKINFO));

   /*
    * We can't just perform a binary comparison of the two ILinkInfos here.  We
    * may have two LinkInfos that refer to the same path, but differ in case on
    * a non-case-sensitive file system.
    */

   /* Compare ILinkInfos by local or remote data. */

   if (IS_FLAG_SET(pciliFirst->dwFlags, ILI_FL_LOCAL_INFO_VALID) &&
       IS_FLAG_SET(pciliSecond->dwFlags, ILI_FL_LOCAL_INFO_VALID))
      /* Compare local data. */
      cr = CompareILinkInfoLocalData(pciliFirst, pciliSecond);
   else if (IS_FLAG_SET(pciliFirst->dwFlags, ILI_FL_REMOTE_INFO_VALID) &&
            IS_FLAG_SET(pciliSecond->dwFlags, ILI_FL_REMOTE_INFO_VALID))
      /* Compare remote data. */
      cr = CompareILinkInfoRemoteData(pciliFirst, pciliSecond);
   else
   {
      /*
       * One contains only valid local information and the other contains only
       * valid remote information.
       */

      ASSERT(! ((pciliFirst->dwFlags & (ILI_FL_LOCAL_INFO_VALID | ILI_FL_REMOTE_INFO_VALID)) &
                (pciliSecond->dwFlags & (ILI_FL_LOCAL_INFO_VALID | ILI_FL_REMOTE_INFO_VALID))));

      /* By fiat, local only < remote only. */

      if (IS_FLAG_SET(pciliFirst->dwFlags, ILI_FL_LOCAL_INFO_VALID))
         cr = CR_FIRST_SMALLER;
      else
         cr = CR_FIRST_LARGER;
   }

   ASSERT(IsValidCOMPARISONRESULT(cr));

   return(cr);
}


/*
** CompareILinkInfoLocalData()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Local ILinkInfo data is compared in the following order:
**
**    1) volume ID
**    2) sub path from root
*/
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoLocalData(PCILINKINFO pciliFirst,
                                                   PCILINKINFO pciliSecond)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pciliFirst, CILINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pciliSecond, CILINKINFO));

   cr = CompareVolumeIDs(ILI_Volume_ID_Ptr(pciliFirst),
                         ILI_Volume_ID_Ptr(pciliSecond));

   if (cr == CR_EQUAL)
      cr = CompareLocalPaths(pciliFirst, pciliSecond);

   ASSERT(IsValidCOMPARISONRESULT(cr));

   return(cr);
}


/*
** CompareLocalPaths()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT CompareLocalPaths(PCILINKINFO pciliFirst,
                                           PCILINKINFO pciliSecond)
{
   COMPARISONRESULT cr;
   TCHAR rgchFirstLocalPath[MAX_PATH_LEN];
   TCHAR rgchSecondLocalPath[MAX_PATH_LEN];

   ASSERT(IS_VALID_STRUCT_PTR(pciliFirst, CILINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pciliSecond, CILINKINFO));

   GetLocalPathFromILinkInfo(pciliFirst, rgchFirstLocalPath);
   GetLocalPathFromILinkInfo(pciliSecond, rgchSecondLocalPath);

   cr = ComparePathStrings(rgchFirstLocalPath, rgchSecondLocalPath);

   ASSERT(IsValidCOMPARISONRESULT(cr));

   return(cr);
}


/*
** CompareILinkInfoRemoteData()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoRemoteData(PCILINKINFO pciliFirst,
                                                    PCILINKINFO pciliSecond)
{
   COMPARISONRESULT cr;
#ifdef UNICODE
   WCHAR szWideCommonPathSuffixFirst[MAX_PATH];
   WCHAR szWideCommonPathSuffixSecond[MAX_PATH];
   LPWSTR pszWideCommonPathSuffixFirst;
   LPWSTR pszWideCommonPathSuffixSecond;
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pciliFirst, CILINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pciliSecond, CILINKINFO));

   cr = CompareCNRLinks(ILI_CNR_Link_Ptr(pciliFirst),
                        ILI_CNR_Link_Ptr(pciliSecond));

#ifdef UNICODE
   if (pciliFirst->ucbHeaderSize == SIZEOF(ILINKINFOA))
   {
      pszWideCommonPathSuffixFirst = szWideCommonPathSuffixFirst;
      MultiByteToWideChar(CP_ACP, 0,
                         ILI_Common_Path_Suffix_PtrA(pciliFirst), -1,
                         szWideCommonPathSuffixFirst, MAX_PATH);
   }
   else
   {
      pszWideCommonPathSuffixFirst = ILI_Common_Path_Suffix_Ptr(pciliFirst);
   }

   if (pciliSecond->ucbHeaderSize == SIZEOF(ILINKINFOA))
   {
      pszWideCommonPathSuffixSecond = szWideCommonPathSuffixSecond;
      MultiByteToWideChar(CP_ACP, 0,
                         ILI_Common_Path_Suffix_PtrA(pciliSecond), -1,
                         szWideCommonPathSuffixSecond, MAX_PATH);
   }
   else
   {
      pszWideCommonPathSuffixSecond = ILI_Common_Path_Suffix_Ptr(pciliSecond);
   }
#else
   if (cr == CR_EQUAL)
      cr = ComparePathStrings(ILI_Common_Path_Suffix_Ptr(pciliFirst),
                              ILI_Common_Path_Suffix_Ptr(pciliSecond));
#endif

   ASSERT(IsValidCOMPARISONRESULT(cr));

   return(cr);
}


/*
** CompareILinkInfoVolumes()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT CompareILinkInfoVolumes(PCILINKINFO pciliFirst,
                                                 PCILINKINFO pciliSecond)
{
   COMPARISONRESULT cr;
   BOOL bFirstLocal;
   BOOL bFirstRemote;
   BOOL bSecondLocal;
   BOOL bSecondRemote;

   ASSERT(IS_VALID_STRUCT_PTR(pciliFirst, CILINKINFO));
   ASSERT(IS_VALID_STRUCT_PTR(pciliSecond, CILINKINFO));

   bFirstLocal = IS_FLAG_SET(((PCILINKINFO)pciliFirst)->dwFlags,
                             ILI_FL_LOCAL_INFO_VALID);
   bFirstRemote = IS_FLAG_SET(((PCILINKINFO)pciliFirst)->dwFlags,
                              ILI_FL_REMOTE_INFO_VALID);

   bSecondLocal = IS_FLAG_SET(((PCILINKINFO)pciliSecond)->dwFlags,
                              ILI_FL_LOCAL_INFO_VALID);
   bSecondRemote = IS_FLAG_SET(((PCILINKINFO)pciliSecond)->dwFlags,
                               ILI_FL_REMOTE_INFO_VALID);

   if (bFirstLocal && bSecondLocal)
      /* First and second have local information. */
      cr = CompareVolumeIDs(ILI_Volume_ID_Ptr((PCILINKINFO)pciliFirst),
                            ILI_Volume_ID_Ptr((PCILINKINFO)pciliSecond));
   else if (bFirstRemote && bSecondRemote)
      /* First and second have remote information. */
      cr = CompareCNRLinks(ILI_CNR_Link_Ptr((PCILINKINFO)pciliFirst),
                           ILI_CNR_Link_Ptr((PCILINKINFO)pciliSecond));
   else
   {
      /*
       * One contains only valid local information and the other contains only
       * valid remote information.
       */

      ASSERT(! ((pciliFirst->dwFlags & (ILI_FL_LOCAL_INFO_VALID | ILI_FL_REMOTE_INFO_VALID)) &
                (pciliSecond->dwFlags & (ILI_FL_LOCAL_INFO_VALID | ILI_FL_REMOTE_INFO_VALID))));

      /* By fiat, local only < remote only. */

      if (bFirstLocal)
         /*
          * First has only local information.  Second has only remote
          * information.
          */
         cr = CR_FIRST_SMALLER;
      else
         /*
          * First has only remote information.  Second has only local
          * information.
          */
         cr = CR_FIRST_LARGER;
   }

   return(cr);
}


/*
** CheckCombinedPathLen()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CheckCombinedPathLen(LPCTSTR pcszBase, LPCTSTR pcszSuffix)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRING_PTR(pcszBase, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSuffix, CSTR));

   bResult = EVAL(lstrlen(pcszBase) + lstrlen(pcszSuffix) < MAX_PATH_LEN);

   if (bResult)
   {
      TCHAR rgchCombinedPath[MAX_PATH_LEN + 1];

      lstrcpy(rgchCombinedPath, pcszBase);
      CatPath(rgchCombinedPath, pcszSuffix);

      bResult = EVAL(lstrlen(rgchCombinedPath) < MAX_PATH_LEN);
   }

   return(bResult);
}


/*
** GetILinkInfoData()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetILinkInfoData(PCILINKINFO pcili, LINKINFODATATYPE lidt,
                              PCVOID *ppcvData)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));
   ASSERT(IsValidLINKINFODATATYPE(lidt));
   ASSERT(IS_VALID_WRITE_PTR(ppcvData, PCVOID));

   switch (lidt)
   {
      case LIDT_VOLUME_SERIAL_NUMBER:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
            bResult = GetVolumeSerialNumber(ILI_Volume_ID_Ptr(pcili),
                                            (PCDWORD *)ppcvData);
         break;

      case LIDT_DRIVE_TYPE:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
            bResult = GetVolumeDriveType(ILI_Volume_ID_Ptr(pcili),
                                         (PCUINT *)ppcvData);
         break;

      case LIDT_VOLUME_LABEL:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
            bResult = GetVolumeLabel(ILI_Volume_ID_Ptr(pcili),
                                     (LPCSTR *)ppcvData);
         break;

      case LIDT_VOLUME_LABELW:
#ifdef UNICODE
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
            bResult = GetVolumeLabelW(ILI_Volume_ID_Ptr(pcili),
                                     (LPCTSTR *)ppcvData);
#endif
         break;

      case LIDT_LOCAL_BASE_PATH:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
         {
            *ppcvData = ILI_Local_Base_Path_PtrA(pcili);
            bResult = TRUE;
         }
         break;

      case LIDT_LOCAL_BASE_PATHW:
#ifdef UNICODE
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
         {
            if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
               *ppcvData = NULL;
            else
               *ppcvData = ILI_Local_Base_Path_PtrW(pcili);
            bResult = TRUE;
         }
#endif
         break;

      case LIDT_NET_TYPE:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
            bResult = GetCNRNetType(ILI_CNR_Link_Ptr(pcili),
                                    (PCDWORD *)ppcvData);
         break;

      case LIDT_NET_RESOURCE:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
            bResult = GetCNRName(ILI_CNR_Link_Ptr(pcili),
                                 (LPCSTR *)ppcvData);
         break;

      case LIDT_NET_RESOURCEW:
#ifdef UNICODE
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
            bResult = GetCNRNameW(ILI_CNR_Link_Ptr(pcili),
                                 (LPCWSTR *)ppcvData);
#endif
         break;

      case LIDT_REDIRECTED_DEVICE:
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
            bResult = GetLastRedirectedDevice(ILI_CNR_Link_Ptr(pcili),
                                              (LPCSTR *)ppcvData);
         break;

      case LIDT_REDIRECTED_DEVICEW:
#ifdef UNICODE
         if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
            bResult = GetLastRedirectedDeviceW(ILI_CNR_Link_Ptr(pcili),
                                              (LPCWSTR *)ppcvData);
#endif
         break;

      case LIDT_COMMON_PATH_SUFFIX:
         *ppcvData = ILI_Common_Path_Suffix_PtrA(pcili);
         bResult = TRUE;
         break;

      case LIDT_COMMON_PATH_SUFFIXW:
#ifdef UNICODE
         if (pcili->ucbHeaderSize == sizeof(ILINKINFOA))
         {
            *ppcvData = NULL;
         }
         else
         {
            *ppcvData = ILI_Common_Path_Suffix_PtrW(pcili);
         }
         bResult = TRUE;
#endif
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("GetILinkInfoData(): Bad LINKINFODATATYPE %d."),
                    lidt));
         break;
   }

   return(bResult);
}


/*
** DisconnectILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL DisconnectILinkInfo(PCILINKINFO pcili)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));

   return(DisconnectFromCNR(ILI_CNR_Link_Ptr(pcili)));
}


#if defined(DEBUG) || defined(EXPV)

/*
** IsValidLINKINFODATATYPE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidLINKINFODATATYPE(LINKINFODATATYPE lidt)
{
   BOOL bResult;

   switch (lidt)
   {
      case LIDT_VOLUME_SERIAL_NUMBER:
      case LIDT_DRIVE_TYPE:
      case LIDT_VOLUME_LABEL:
      case LIDT_VOLUME_LABELW:
      case LIDT_LOCAL_BASE_PATH:
      case LIDT_LOCAL_BASE_PATHW:
      case LIDT_NET_TYPE:
      case LIDT_NET_RESOURCE:
      case LIDT_REDIRECTED_DEVICE:
      case LIDT_COMMON_PATH_SUFFIX:
      case LIDT_COMMON_PATH_SUFFIXW:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidLINKINFODATATYPE(): Invalid LINKINFODATATYPE %d."),
                    lidt));
         break;
   }

   return(bResult);
}

#endif


#if defined(DEBUG) || defined(VSTF)

/*
** CheckILIFlags()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CheckILIFlags(PCILINKINFO pcili)
{
   return(FLAGS_ARE_VALID(pcili->dwFlags, ALL_ILINKINFO_FLAGS) &&
          (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID) ||
           IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID)));
}


/*
** CheckILICommonPathSuffix()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CheckILICommonPathSuffix(PCILINKINFO pcili)
{
   return(IS_VALID_STRING_PTRA(ILI_Common_Path_Suffix_PtrA(pcili), CSTR) &&
          EVAL(IsContained(pcili, pcili->li.ucbSize,
                           ILI_Common_Path_Suffix_PtrA(pcili),
                           lstrlenA(ILI_Common_Path_Suffix_PtrA(pcili)))) &&
          EVAL(! IS_SLASH(*ILI_Common_Path_Suffix_PtrA(pcili))));
}


/*
** CheckILILocalInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CheckILILocalInfo(PCILINKINFO pcili)
{
#ifdef UNICODE
   WCHAR   szWideLocalBasePath[MAX_PATH];
   WCHAR   szWideCommonPathSuffix[MAX_PATH];
   LPWSTR  pszWideLocalBasePath;
   LPWSTR  pszWideCommonPathSuffix;

   if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
      return FALSE;
   if (!IS_VALID_STRUCT_PTR(ILI_Volume_ID_Ptr(pcili), CVOLUMEID))
      return FALSE;
   if (!EVAL(IsContained(pcili, pcili->li.ucbSize,ILI_Volume_ID_Ptr(pcili),
                        GetVolumeIDLen(ILI_Volume_ID_Ptr(pcili)))))
      return FALSE;

   if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
   {
      pszWideLocalBasePath = szWideLocalBasePath;
      MultiByteToWideChar(CP_ACP, 0,
                          ILI_Local_Base_Path_PtrA(pcili), -1,
                          szWideLocalBasePath, MAX_PATH);

      pszWideCommonPathSuffix = szWideCommonPathSuffix;
      MultiByteToWideChar(CP_ACP, 0,
                          ILI_Common_Path_Suffix_PtrA(pcili), -1,
                          szWideCommonPathSuffix, MAX_PATH);

   }
   else
   {
      pszWideLocalBasePath = ILI_Local_Base_Path_Ptr(pcili);
      pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
   }

   if (!EVAL(IsDrivePath(pszWideLocalBasePath)))
      return FALSE;
   if (!EVAL(IsContained(pcili, pcili->li.ucbSize,
                           ILI_Local_Base_Path_PtrA(pcili),
                           lstrlenA(ILI_Local_Base_Path_PtrA(pcili)))))
      return FALSE;
   if (!EVAL(CheckCombinedPathLen(pszWideLocalBasePath,
                                  pszWideCommonPathSuffix)))
      return FALSE;

   return TRUE;
#else
   return(IS_FLAG_CLEAR(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID) ||

           /* Check volume ID. */

          (IS_VALID_STRUCT_PTR(ILI_Volume_ID_Ptr(pcili), CVOLUMEID) &&
           EVAL(IsContained(pcili, pcili->li.ucbSize,
                            ILI_Volume_ID_Ptr(pcili),
                            GetVolumeIDLen(ILI_Volume_ID_Ptr(pcili)))) &&

           /* Check local base path. */

           EVAL(IsDrivePath(ILI_Local_Base_Path_Ptr(pcili))) &&
           EVAL(IsContained(pcili, pcili->li.ucbSize,
                            ILI_Local_Base_Path_PtrA(pcili),
                            lstrlen(ILI_Local_Base_Path_Ptr(pcili)))) &&
           EVAL(CheckCombinedPathLen(ILI_Local_Base_Path_Ptr(pcili),
                                   ILI_Common_Path_Suffix_Ptr(pcili)))));
#endif
}


/*
** CheckILIRemoteInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL CheckILIRemoteInfo(PCILINKINFO pcili)
{
   BOOL bResult;

   if (IS_FLAG_CLEAR(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
      bResult = TRUE;
   else
   {
      /* Check CNR link. */

      if (IS_VALID_STRUCT_PTR(ILI_CNR_Link_Ptr(pcili), CCNRLINK) &&
          EVAL(IsContained(pcili, pcili->li.ucbSize,
                           ILI_CNR_Link_Ptr(pcili),
                           GetCNRLinkLen(ILI_CNR_Link_Ptr(pcili)))))
      {
         TCHAR rgchRemoteBasePath[MAX_PATH_LEN];
#ifdef UNICODE
         WCHAR szWideCommonPathSuffix[MAX_PATH];
         LPWSTR pszWideCommonPathSuffix;
#endif
         /* RAIDRAID: (15724) This is broken for non-UNC CNR names. */

         GetRemotePathFromCNRLink(ILI_CNR_Link_Ptr(pcili), rgchRemoteBasePath);

#ifdef UNICODE
         if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
         {
            pszWideCommonPathSuffix = szWideCommonPathSuffix;
            MultiByteToWideChar(CP_ACP, 0,
                                ILI_Common_Path_Suffix_PtrA(pcili), -1,
                                szWideCommonPathSuffix, MAX_PATH);
         }
         else
         {
            pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
         }
         bResult = EVAL(CheckCombinedPathLen(rgchRemoteBasePath,
                                          pszWideCommonPathSuffix));
#else
         bResult = EVAL(CheckCombinedPathLen(rgchRemoteBasePath,
                                          ILI_Common_Path_Suffix_Ptr(pcili)));
#endif

      }
      else
         bResult = FALSE;
   }

   return(bResult);
}


/*
** IsValidPCLINKINFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCLINKINFO(PCLINKINFO pcli)
{
   return(IS_VALID_STRUCT_PTR((PCILINKINFO)pcli, CILINKINFO));
}


/*
** IsValidPCILINKINFO()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCILINKINFO(PCILINKINFO pcili)
{
   /*
    * A "valid" LinkInfo structure has the following characteristics:
    *
    * 1) entire structure is readable
    * 2) size of ILINKINFO header structure >= SIZEOF(CILINKINFO)
    * 3) flags are valid
    * 4) either local info or remote info or both are valid
    * 5) contained structures and strings are valid and are entirely contained
    *    in LinkInfo structure
    * 6) lstrlen() of combined paths < MAX_PATH_LEN
    */

   return(IS_VALID_READ_PTR(pcili, CILINKINFO) &&
          IS_VALID_READ_BUFFER_PTR(pcili, CILINKINFO, pcili->li.ucbSize) &&
          EVAL(pcili->ucbHeaderSize >= SIZEOF(*pcili)) &&
          EVAL(CheckILIFlags(pcili)) &&
          EVAL(CheckILICommonPathSuffix(pcili)) &&
          EVAL(CheckILILocalInfo(pcili)) &&
          EVAL(CheckILIRemoteInfo(pcili)));
}


#ifdef SKIP_OLD_LINKINFO_QUIETLY

/*
** IsNewLinkInfoHackCheck()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsNewLinkInfoHackCheck(PCILINKINFO pcili)
{
   BOOL bResult = FALSE;

   /*
    * HACKHACK: We have added the ucbHeaderSize field to the ILINKINFO
    * structure.  The second DWORD in old ILINKINFO structures was the dwFlags
    * field.  We detect an old ILINKINFO structure as one whose ucbHeaderSize
    * field is not the value we expect.
    *
    * HACKHACK: We have added dwFlags and chLastRedirectedDrive to the ICRNLINK
    * structure.  The second DWORD in old ICNRLINK structures was the
    * ucbNetNameOffset field.  We detect an old ICNRLINK structure as one whose
    * old ucbNetNameOffset field was 8, since old ICRNLINK structures only
    * contain ucbSize, ucbNetNameOffset, and the CNR name string.
    *
    * HACKHACK: We have removed the dwFlags field from the IVOLUMEID structure.
    * The second DWORD in old IVOLUMEID structures was the dwFlags field.  We
    * detect an old IVOLUMEID structure as one whose old dwFlags field was 1,
    * representing the only old IVOLUMEID flag IVOLID_FL_VALID_VOLUME_INFO.
    */

   ASSERT((PDWORD)&(pcili->ucbHeaderSize) == ((PDWORD)(pcili)) + 1);

   if (IS_VALID_READ_PTR(pcili, CILINKINFO))
   {
      if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA
          || pcili->ucbHeaderSize == SIZEOF(ILINKINFOW) )
      {
         if (IS_FLAG_CLEAR(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID) ||
             *((PDWORD)(ILI_CNR_Link_Ptr(pcili)) + 1) != 8)
         {
            if (IS_FLAG_CLEAR(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID) ||
                *((PDWORD)(ILI_Volume_ID_Ptr(pcili)) + 1) != 1)
               bResult = TRUE;
            else
               WARNING_OUT((TEXT("IsNewLinkInfoHackCheck(): Failing LinkInfo API called on old LinkInfo structure with dwFlags field in IVOLUMEID sub-structure.")));
         }
         else
            WARNING_OUT((TEXT("IsNewLinkInfoHackCheck(): Failing LinkInfo API called on old LinkInfo structure without dwFlags and chLastRedirectedDrive fields in ICNRLINK sub-structure.")));
      }
      else
         WARNING_OUT((TEXT("IsNewLinkInfoHackCheck(): Failing LinkInfo API called on old LinkInfo structure without ucbHeaderSize field.")));
   }

   return(bResult);
}

#endif

#endif


#ifdef DEBUG

/*
** DumpILinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void DumpILinkInfo(PCILINKINFO pcili)
{
#ifdef UNICODE
   WCHAR   szWideCommonPathSuffix[MAX_PATH];
   LPWSTR  pszWideCommonPathSuffix;
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pcili, CILINKINFO));

   PLAIN_TRACE_OUT((TEXT("%s[LinkInfo] ucbSize = %#x"),
                    INDENT_STRING,
                    pcili->li.ucbSize));
   PLAIN_TRACE_OUT((TEXT("%s%s[ILinkInfo] ucbHeaderSize = %#x"),
                    INDENT_STRING,
                    INDENT_STRING,
                    pcili->ucbHeaderSize));
   PLAIN_TRACE_OUT((TEXT("%s%s[ILinkInfo] dwFLags = %#08lx"),
                    INDENT_STRING,
                    INDENT_STRING,
                    pcili->dwFlags));

   if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_LOCAL_INFO_VALID))
   {
#ifdef UNICODE
      WCHAR   szWideLocalBasePath[MAX_PATH];
      LPWSTR  pszWideLocalBasePath;
#endif
      DumpVolumeID(ILI_Volume_ID_Ptr(pcili));
#ifdef UNICODE
      if (pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
      {
         pszWideLocalBasePath = szWideLocalBasePath;
         MultiByteToWideChar(CP_ACP, 0,
                             ILI_Local_Base_Path_PtrA(pcili), -1,
                             szWideLocalBasePath, MAX_PATH);
      }
      else
      {
         pszWideLocalBasePath = ILI_Local_Base_Path_Ptr(pcili);
      }
      PLAIN_TRACE_OUT((TEXT("%s%s[ILinkInfo] local base path \"%s\""),
                       INDENT_STRING,
                       INDENT_STRING,
                       pszWideLocalBasePath));
#else
      PLAIN_TRACE_OUT((TEXT("%s%s[ILinkInfo] local base path \"%s\""),
                       INDENT_STRING,
                       INDENT_STRING,
                       ILI_Local_Base_Path_Ptr(pcili)));
#endif
   }

   if (IS_FLAG_SET(pcili->dwFlags, ILI_FL_REMOTE_INFO_VALID))
      DumpCNRLink(ILI_CNR_Link_Ptr(pcili));

#ifdef UNICODE
   if ( pcili->ucbHeaderSize == SIZEOF(ILINKINFOA))
   {
     pszWideCommonPathSuffix = szWideCommonPathSuffix;
     MultiByteToWideChar(CP_ACP, 0,
                         ILI_Common_Path_Suffix_PtrA(pcili), -1,
                         szWideCommonPathSuffix, MAX_PATH);

   }
   else
   {
      pszWideCommonPathSuffix = ILI_Common_Path_Suffix_Ptr(pcili);
   }

   PLAIN_TRACE_OUT((TEXT("%s%s[ILinkInfo] common path suffix \"%s\""),
                    INDENT_STRING,
                    INDENT_STRING,
                    pszWideCommonPathSuffix));
#else
   PLAIN_TRACE_OUT((TEXT("%s%s[ILinkInfo] common path suffix \"%s\""),
                    INDENT_STRING,
                    INDENT_STRING,
                    ILI_Common_Path_Suffix_Ptr(pcili)));
#endif
   return;
}

#endif


/***************************** Exported Functions ****************************/


/******************************************************************************

@doc LINKINFOAPI

@func BOOL | CreateLinkInfo | Creates a LinkInfo structure for a path.

@parm PCSTR | pcszPath | A pointer to the path string that a LinkInfo structure
is to be created for.

@parm PLINKINFO * | ppli | A pointer to a PLINKINFO to be filled in with a
pointer to the new LinkInfo structure.  *ppli is only valid if TRUE is
returned.

@rdesc If a LinkInfo structure was created successfully, TRUE is returned, and
*ppli contains a pointer to the new LinkInfo structure.  Otherwise, a LinkInfo
structure was not created successfully, and *ppli is undefined.  The reason for
failure may be determined by calling GetLastError().

@comm Once the caller is finshed with the LinkInfo structure returned by
CreateLinkInfo(), DestroyLinkInfo() should be called to free the LinkInfo
structure.<nl>
The contents of the LinkInfo structure returned are opaque to the caller, with
the exception of the first field of the LinkInfo structure.  The first field of
the LinkInfo structure, ucbSize, is a UINT containing the size of the LinkInfo
structure in bytes, including the ucbSize field.<nl>
The LinkInfo structure is created in memory that is private to the LinkInfo
APIs.  The returned LinkInfo structure should be copied into the caller's
memory, and the DestroyLinkInfo() should be called to free the LinkInfo
structure from the LinkInfo APIs' private memory.

@xref DestroyLinkInfo

******************************************************************************/

LINKINFOAPI BOOL WINAPI CreateLinkInfo(LPCTSTR pcszPath, PLINKINFO *ppli)
{
   BOOL bResult;

   DebugEntry(CreateLinkInfo);

#ifdef EXPV
   /* Verify parameters. */

   if (IS_VALID_STRING_PTR(pcszPath, CSTR) &&
       IS_VALID_WRITE_PTR(ppli, PLINKINFO))
#endif
   {
      bResult = CreateILinkInfo(pcszPath, (PILINKINFO *)ppli);

#ifdef DEBUG

      if (bResult)
      {
         TRACE_OUT((TEXT("CreateLinkInfo(): LinkInfo created for path %s:"),
                    pcszPath));
         DumpILinkInfo(*(PILINKINFO *)ppli);
      }

#endif

   }
#ifdef EXPV
   else
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      bResult = FALSE;
   }
#endif

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR(*ppli, CLINKINFO));

   DebugExitBOOL(CreateLinkInfo, bResult);

   return(bResult);
}

#ifdef UNICODE
LINKINFOAPI BOOL WINAPI CreateLinkInfoA(LPCSTR pcszPath, PLINKINFO *ppli)
{
    LPWSTR  lpwstr;
    UINT    cchPath;

    cchPath = lstrlenA(pcszPath) + 1;

    lpwstr = (LPWSTR)_alloca(cchPath*SIZEOF(WCHAR));

    if ( MultiByteToWideChar( CP_ACP, 0,
                              pcszPath, cchPath,
                              lpwstr, cchPath) == 0)
    {
        return FALSE;
    }
    else
    {
        return CreateLinkInfo(lpwstr,ppli);
    }
}
#endif

/******************************************************************************

@doc LINKINFOAPI

@func void | DestroyLinkInfo | Destroys a LinkInfo structure created by
CreateLinkInfo().

@parm PLINKINFO | pli | A pointer to the LinkInfo structure to be destroyed.

@xref CreateLinkInfo

******************************************************************************/

LINKINFOAPI void WINAPI DestroyLinkInfo(PLINKINFO pli)
{
   DebugEntry(DestroyLinkInfo);

#ifdef EXPV
   /* Verify parameters. */

   if (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
       IsNewLinkInfoHackCheck((PCILINKINFO)pli) &&
#endif
       IS_VALID_STRUCT_PTR(pli, CLINKINFO))
#endif
   {
      DestroyILinkInfo((PILINKINFO)pli);
   }

   DebugExitVOID(DestroyLinkInfo);

   return;
}


/******************************************************************************

@doc LINKINFOAPI

@func int | CompareLinkInfoReferents | Compares the referents of two LinkInfo
structures.

@parm PCLINKINFO | pcliFirst | A pointer to the first LinkInfo structure whose
referent is to be compared.

@parm PCLINKINFO | pcliSecond | A pointer to the second LinkInfo structure
whose referent is to be compared.

@rdesc If the referent of the first LinkInfo structure is less than the
referent of the second LinkInfo structure, a negative value is returned.  If
the referent of the first LinkInfo structure is the same as the referent of the
second LinkInfo structure, zero is returned.  If the referent of the first
LinkInfo structure is larger than the referent of the second LinkInfo
structure, a positive value is returned.  An invalid LinkInfo structure is
considered to have a referent that is less than the referent of any valid
LinkInfo structure.  All invalid LinkInfo structures are considered to have the
same referent.

@comm The value returned is actually a COMPARISONRESULT, for clients that
understand COMPARISONRESULTs, like SYNCENG.DLL.

@xref CompareLinkInfoVolumes

******************************************************************************/

LINKINFOAPI int WINAPI CompareLinkInfoReferents(PCLINKINFO pcliFirst,
                                                PCLINKINFO pcliSecond)
{
   COMPARISONRESULT cr;
   BOOL bFirstValid;
   BOOL bSecondValid;

   DebugEntry(CompareLinkInfoReferents);

   bFirstValid = (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
                  IsNewLinkInfoHackCheck((PCILINKINFO)pcliFirst) &&
#endif
                  IS_VALID_STRUCT_PTR(pcliFirst, CLINKINFO));

   bSecondValid = (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
                  IsNewLinkInfoHackCheck((PCILINKINFO)pcliSecond) &&
#endif
                  IS_VALID_STRUCT_PTR(pcliSecond, CLINKINFO));

   if (bFirstValid)
   {
      if (bSecondValid)
         cr = CompareILinkInfoReferents((PCILINKINFO)pcliFirst,
                                        (PCILINKINFO)pcliSecond);
      else
         cr = CR_FIRST_LARGER;
   }
   else
   {
      if (bSecondValid)
         cr = CR_FIRST_SMALLER;
      else
         cr = CR_EQUAL;
   }

   ASSERT(IsValidCOMPARISONRESULT(cr));

   DebugExitCOMPARISONRESULT(CompareLinkInfoReferents, cr);

   return(cr);
}


/******************************************************************************

@doc LINKINFOAPI

@func int | CompareLinkInfoVolumes | Compares the volumes of the referents of
two LinkInfo structures.

@parm PCLINKINFO | pcliFirst | A pointer to the first LinkInfo structure whose
referent's volume is to be compared.

@parm PCLINKINFO | pcliSecond | A pointer to the second LinkInfo structure
referent's volume is to be compared.

@rdesc If the volume of the referent of the first LinkInfo structure is less
than the volume of the referent of the second LinkInfo structure, a negative
value is returned.  If the volume of the referent of the first LinkInfo
structure is the same as the volume of the referent of the second LinkInfo
structure, zero is returned.  If the volume of the referent of the first
LinkInfo structure is larger than the volume of the referent of the second
LinkInfo structure, a positive value is returned.  An invalid LinkInfo
structure is considered to have a referent's volume that is less than the
referent's volume of any valid LinkInfo structure.  All invalid LinkInfo
structures are considered to have the same referent's volume.

@comm The value returned is actually a COMPARISONRESULT, for clients that
understand COMPARISONRESULTs, like SYNCENG.DLL.

@xref CompareLinkInfoReferents

******************************************************************************/

LINKINFOAPI int WINAPI CompareLinkInfoVolumes(PCLINKINFO pcliFirst,
                                              PCLINKINFO pcliSecond)
{
   COMPARISONRESULT cr;
   BOOL bFirstValid;
   BOOL bSecondValid;

   DebugEntry(CompareLinkInfoVolumes);

   bFirstValid = (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
                  IsNewLinkInfoHackCheck((PCILINKINFO)pcliFirst) &&
#endif
                  IS_VALID_STRUCT_PTR(pcliFirst, CLINKINFO));

   bSecondValid = (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
                  IsNewLinkInfoHackCheck((PCILINKINFO)pcliSecond) &&
#endif
                  IS_VALID_STRUCT_PTR(pcliSecond, CLINKINFO));

   if (bFirstValid)
   {
      if (bSecondValid)
         cr = CompareILinkInfoVolumes((PCILINKINFO)pcliFirst,
                                      (PCILINKINFO)pcliSecond);
      else
         cr = CR_FIRST_LARGER;
   }
   else
   {
      if (bSecondValid)
         cr = CR_FIRST_SMALLER;
      else
         cr = CR_EQUAL;
   }

   ASSERT(IsValidCOMPARISONRESULT(cr));

   DebugExitCOMPARISONRESULT(CompareLinkInfoVolumes, cr);

   return(cr);
}


/******************************************************************************

@doc LINKINFOAPI

@func BOOL | ResolveLinkInfo | Resolves a LinkInfo structure into a file system
path on an available volume.

@parm PCLINKINFO | pcli | A pointer to the LinkInfo structure to be resolved.

@parm PSTR | pszResolvedPathBuf | A pointer to a buffer to be filled in with
the path resolved to the LinkInfo structure's referent.

@parm DWORD | dwInFlags | A bit mask of flags.  This parameter may be any
combination of the following values:

@flag RLI_IFL_CONNECT | If set, connect to the referent's parent connectable
network resource if necessary.  If clear, no connection is established.

@flag RLI_IFL_ALLOW_UI | If set, interaction with the user is permitted, and
the hwndOwner parameter identifies the parent window to be used for any ui
required.  If clear, interaction with the user is not permitted.

@flag RLI_IFL_REDIRECT | If set, the resolved path is a redirected logical
device path.  If clear, the resolved path is only a redirected logical device
path if the RLI_IFL_CONNECT flag is set, and the network requires a redirected
logical device path to make a connection.

@flag RLI_IFL_UPDATE | If set and the source LinkInfo structure needs updating,
RLI_OFL_UPDATED will be set in *pdwOutFlags and *ppliUpdated will point to an
updated LinkInfo structure.  If clear, RLI_OFL_UPDATED will be clear in
*pdwOutFlags and *ppliUpdated is undefined.

@flag RLI_IFL_LOCAL_SEARCH | If set, first the last known logical device for
the referent's volume is checked for the volume, followed by all other local
logical devices that handle the referent's volume's media type.  If clear, only
the last known logical device for the referent's volume is checked for the
volume.

@parm HWND | hwndOwner | A handle to the parent window to be used to bring up
any ui required.  This parameter is only used if RLI_IFL_ALLOW_UI is set in
dwInFlags.  Otherwise, it is ignored.

@parm PDWORD | pdwOutFlags | A pointer to a DWORD to be filled in with a bit
mask of flags. *pdwOutFlags is only valid if TRUE is returned.  *pdwOutFlags
may be any combination of the following values:

@flag RLI_OFL_UPDATED | Only set if RLI_IFL_UPDATE was set in dwInFlags.  If
set, the source LinkInfo structure needed updating, and *ppliUpdated points to
an updated LinkInfo structure.  If clear, either RLI_IFL_UPDATE was clear in
dwInFlags or the source LinkInfo structure didn't need updating, and
*ppliUpdated is undefined.

@parm PLINKINFO * | ppliUpdated | If RLI_IFL_UPDATE is set in dwInFlags,
ppliUpdated is a pointer to a PLINKINFO to be filled in with a pointer to an
updated LinkInfo structure, if necessary.  If RLI_IFL_UPDATE is clear in
dwInFlags, ppliUpdated is ignored.  *ppliUpdated is only valid if
RLI_OFL_UPDATED is set in *pdwOutFlags

@rdesc If the LinkInfo was resolved to a path on an available successfully,
TRUE is returned, pszResolvedPathBuf's buffer is filled in with a file system
path to the LinkInfo structure's referent, and *pdwOutFlags is filled in as
described above.  Otherwise, FALSE is returned, the contents of pszResolved's
buffer are undefined, and the contents of *pdwOutFlags are undefined.  The
reason for failure may be determined by calling GetLastError().

@comm Once the caller is finshed with any new, updated LinkInfo structure
returned by ResolveLinkInfo(), DestroyLinkInfo() should be called to free the
LinkInfo structure.

@xref DestroyLinkInfo DisconnectLinkInfo

******************************************************************************/

LINKINFOAPI BOOL WINAPI ResolveLinkInfo(PCLINKINFO pcli,
                                        LPTSTR pszResolvedPathBuf,
                                        DWORD dwInFlags, HWND hwndOwner,
                                        PDWORD pdwOutFlags,
                                        PLINKINFO *ppliUpdated)
{
   BOOL bResult;

   DebugEntry(ResolveLinkInfo);

#ifdef EXPV
   /* Verify parameters. */

   if (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
       IsNewLinkInfoHackCheck((PCILINKINFO)pcli) &&
#endif
       IS_VALID_STRUCT_PTR(pcli, CLINKINFO) &&
       IS_VALID_WRITE_BUFFER_PTR(pszResolvedPathBuf, STR, MAX_PATH_LEN) &&
       FLAGS_ARE_VALID(dwInFlags, ALL_RLI_IFLAGS) &&
       (IS_FLAG_CLEAR(dwInFlags, RLI_IFL_ALLOW_UI) ||
        IS_VALID_HANDLE(hwndOwner, WND)) &&
       IS_VALID_WRITE_PTR(pdwOutFlags, DWORD) &&
       (IS_FLAG_CLEAR(dwInFlags, RLI_IFL_UPDATE) ||
        IS_VALID_WRITE_PTR(ppliUpdated, PLINKINFO)) &&
       EVAL(IS_FLAG_CLEAR(dwInFlags, RLI_IFL_TEMPORARY) ||
            IS_FLAG_SET(dwInFlags, RLI_IFL_CONNECT)))
#endif
   {
      DWORD dwTempFlags;

      *pdwOutFlags = 0;

      bResult = ResolveILinkInfo((PCILINKINFO)pcli, pszResolvedPathBuf,
                                 dwInFlags, hwndOwner, &dwTempFlags);

      if (bResult)
      {
         *pdwOutFlags |= dwTempFlags;

         if (IS_FLAG_SET(dwInFlags, RLI_IFL_UPDATE))
         {
            bResult = UpdateILinkInfo((PCILINKINFO)pcli, pszResolvedPathBuf,
                                      &dwTempFlags,
                                      (PILINKINFO *)ppliUpdated);

            if (bResult)
               *pdwOutFlags |= dwTempFlags;
         }
      }

#ifdef DEBUG

      TRACE_OUT((TEXT("ResolveLinkInfo(): flags %#08lx, given LinkInfo:"),
                 dwInFlags));
      DumpILinkInfo((PCILINKINFO)pcli);

      if (bResult)
      {
         TRACE_OUT((TEXT("ResolveLinkInfo(): Resolved path %s with flags %#08lx."),
                    pszResolvedPathBuf,
                    *pdwOutFlags));

         if (IS_FLAG_SET(*pdwOutFlags, RLI_OFL_UPDATED))
         {
            ASSERT(IS_FLAG_SET(dwInFlags, RLI_IFL_UPDATE));

            TRACE_OUT((TEXT("UpdateLinkInfo(): updated LinkInfo:")));
            DumpILinkInfo(*(PILINKINFO *)ppliUpdated);
         }
         else
         {
            if (IS_FLAG_SET(dwInFlags, RLI_IFL_UPDATE))
               TRACE_OUT((TEXT("UpdateLinkInfo(): No update required.")));
            else
               TRACE_OUT((TEXT("UpdateLinkInfo(): No update requested.")));
         }
      }
      else
         WARNING_OUT((TEXT("ResolveLinkInfo(): Referent's volume is unavailable.")));

#endif

   }
#ifdef EXPV
   else
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      bResult = FALSE;
   }
#endif

   ASSERT(! bResult ||
          (FLAGS_ARE_VALID(*pdwOutFlags, ALL_RLI_OFLAGS) &&
           EVAL(IsCanonicalPath(pszResolvedPathBuf)) &&
           EVAL(! (IS_FLAG_CLEAR(dwInFlags, RLI_IFL_UPDATE) &&
                   IS_FLAG_SET(*pdwOutFlags, RLI_OFL_UPDATED))) &&
           (IS_FLAG_CLEAR(*pdwOutFlags, RLI_OFL_UPDATED) ||
            IS_VALID_STRUCT_PTR(*ppliUpdated, CLINKINFO))));

   DebugExitBOOL(ResolveLinkInfo, bResult);

   return(bResult);
}

#ifdef UNICODE
LINKINFOAPI BOOL WINAPI ResolveLinkInfoA(PCLINKINFO pcli,
                                        LPSTR pszResolvedPathBuf,
                                        DWORD dwInFlags, HWND hwndOwner,
                                        PDWORD pdwOutFlags,
                                        PLINKINFO *ppliUpdated)
{
    WCHAR   szWideResolvedPathBuf[MAX_PATH];
    BOOL    fResolved;

    fResolved = ResolveLinkInfo(pcli,
                                szWideResolvedPathBuf,
                                dwInFlags, hwndOwner, pdwOutFlags, ppliUpdated);
    if ( fResolved )
    {
        if ( WideCharToMultiByte( CP_ACP, 0,
                                  szWideResolvedPathBuf, -1,
                                  pszResolvedPathBuf, MAX_PATH,
                                  NULL, NULL ) == 0)
        {
            return FALSE;
        }
    }
    return fResolved;
}
#endif

/******************************************************************************

@doc LINKINFOAPI

@func BOOL | DisconnectLinkInfo | Cancels a connection to a net resource
established by a previous call to ResolveLinkInfo().  DisconnectLinkInfo()
should only be called if RLI_OFL_DISCONNECT was set in *pdwOutFlags on return
from ResolveLinkInfo() on the given LinkInfo structure, or its updated
equivalent.

@parm PCLINKINFO | pcli | A pointer to the LinkInfo structure whose connection
is to be canceled.

@rdesc If the function completed successfully, TRUE is returned.  Otherwise,
FALSE is returned.  The reason for failure may be determined by calling
GetLastError().

@xref ResolveLinkInfo

******************************************************************************/

LINKINFOAPI BOOL WINAPI DisconnectLinkInfo(PCLINKINFO pcli)
{
   BOOL bResult;

   DebugEntry(DisconnectLinkInfo);

#ifdef EXPV
   /* Verify parameters. */

   if (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
       IsNewLinkInfoHackCheck((PCILINKINFO)pcli) &&
#endif
       IS_VALID_STRUCT_PTR(pcli, CLINKINFO))
#endif
   {
      bResult = DisconnectILinkInfo((PCILINKINFO)pcli);
   }
#ifdef EXPV
   else
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      bResult = FALSE;
   }
#endif

   DebugExitBOOL(DisconnectLinkInfo, bResult);

   return(bResult);
}


/******************************************************************************

@doc LINKINFOAPI

@func BOOL | GetLinkInfoData | Retrieves a pointer to data in a LinkInfo
structure.

@parm PCLINKINFO | pcli | A pointer to the LinkInfo structure to retrieve data
from.

@parm LINKINFODATATYPE | lidt | The type of data to be retrieved from the
LinkInfo structure.  lidt may be one of the following values:

@flag LIDT_VOLUME_SERIAL_NUMBER | *ppcvData is a PCDWORD that points to the
LinkInfo structure's referent's volume's serial number.

@flag LIDT_DRIVE_TYPE | *ppcvData is a PCUINT that points to the LinkInfo
structure's referent's volume's host drive type.

@flag LIDT_VOLUME_LABEL | *ppcvData is a PCSTR that points to the LinkInfo
structure's referent's volume's label.

@flag LIDT_LOCAL_BASE_PATH | *ppcvData is a PCSTR that points to the LinkInfo
structure's referent's local base path.

@flag LIDT_NET_RESOURCE | *ppcvData is a PCSTR that points to the LinkInfo
structure's referent's parent network resource's name.

@flag LIDT_COMMON_PATH_SUFFIX | *ppcvData is a PCSTR that points to the
LinkInfo structure's referent's common path suffix.

@rdesc If the function completed successfully, TRUE is returned, and *ppcvData
is filled in with a pointer to the data requested from LinkInfo structure.
Otherwise, FALSE is returned, and the contents of *ppcvData are undefined.  The
reason for failure may be determined by calling GetLastError().

@comm A LinkInfo structure may only contain some of the LinkInfo data listed
above.

******************************************************************************/

LINKINFOAPI BOOL WINAPI GetLinkInfoData(PCLINKINFO pcli, LINKINFODATATYPE lidt,
                                        PCVOID *ppcvData)
{
   BOOL bResult;

   DebugEntry(GetLinkInfoData);

#ifdef EXPV
   /* Verify parameters. */

   if (
#ifdef SKIP_OLD_LINKINFO_QUIETLY
       IsNewLinkInfoHackCheck((PCILINKINFO)pcli) &&
#endif
       IS_VALID_STRUCT_PTR(pcli, CLINKINFO) &&
       EVAL(IsValidLINKINFODATATYPE(lidt)) &&
       IS_VALID_WRITE_PTR(ppcvData, PCVOID))
#endif
   {
      bResult = GetILinkInfoData((PCILINKINFO)pcli, lidt, ppcvData);
   }
#ifdef EXPV
   else
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      bResult = FALSE;
   }
#endif

   ASSERT(! bResult ||
          IS_VALID_READ_BUFFER_PTR(*ppcvData, LinkInfoData, 1));

   DebugExitBOOL(GetLinkInfoData, bResult);

   return(bResult);
}


/******************************************************************************

@doc LINKINFOAPI

@func BOOL | IsValidLinkInfo | Determines whether or not a LinkInfo structure
is valid.

@parm PCLINKINFO | pcli | A pointer to the LinkInfo structure to be checked for
validity.

@rdesc If the function completed successfully, TRUE is returned.  Otherwise,
FALSE is returned.

******************************************************************************/

LINKINFOAPI BOOL WINAPI IsValidLinkInfo(PCLINKINFO pcli)
{
   BOOL bResult;

   DebugEntry(IsValidLinkInfo);

#ifdef SKIP_OLD_LINKINFO_QUIETLY

   bResult = (IsNewLinkInfoHackCheck((PCILINKINFO)pcli) &&
              IS_VALID_STRUCT_PTR(pcli, CLINKINFO));

#else

   bResult = IS_VALID_STRUCT_PTR(pcli, CLINKINFO);

#endif

   DebugExitBOOL(IsValidLinkInfo, bResult);

   return(bResult);
}
