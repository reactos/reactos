/*
 * volumeid.c - Volume ID ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "volumeid.h"


/* Constants
 ************/

/* local root path constants */

#define MAX_LOCAL_DRIVES            (TEXT('z') - TEXT('a') + 1)


/* Macros
 *********/

/* macros for accessing IVOLUMEID data */

#define IVOLID_Volume_Label_PtrA(pivolid) \
   ((LPSTR)(((PBYTE)(pivolid)) + (pivolid)->ucbVolumeLabelOffset))

#ifdef UNICODE
#define IVOLID_Volume_Label_PtrW(pivolid) \
   ((LPTSTR)(((PBYTE)(pivolid)) + (pivolid)->ucbVolumeLabelOffsetW))
#endif

#ifdef UNICODE
#define IVOLID_Volume_Label_Ptr(pivolid)   IVOLID_Volume_Label_PtrW(pivolid)
#else
#define IVOLID_Volume_Label_Ptr(pivolid)   IVOLID_Volume_Label_PtrA(pivolid)
#endif

/* Types
 ********/

/*
   @doc INTERNAL

   @struct IVOLUMEID | Internal definition of relocatable volume ID structure.
   An <t ILINKINFO> structure may contain an IVOLUMEID structure.  An IVOLUMEID
   structure consists of a header described as below, followed by
   variable-length data.
*/

typedef struct _ivolumeidA
{
   /*
      @field UINT | ucbSize | Length of IVOLUMEID structure in bytes, including
      ucbSize field.
   */

   UINT ucbSize;

   /*
      @field UINT | uDriveType | The volume's host drive type, as returned by
      GetDriveType()
   */

   UINT uDriveType;

   /* @field DWORD | dwSerialNumber | The volume's serial number. */

   DWORD dwSerialNumber;

   /*
      @field UINT | ucbVolumeLabelOffset | Offset in bytes of volume label
      string from base of structure.
   */

   UINT ucbVolumeLabelOffset;
}
IVOLUMEIDA;
DECLARE_STANDARD_TYPES(IVOLUMEIDA);

#ifdef UNICODE
typedef struct _ivolumeidW
{
   /*
      @field UINT | ucbSize | Length of IVOLUMEID structure in bytes, including
      ucbSize field.
   */

   UINT ucbSize;

   /*
      @field UINT | uDriveType | The volume's host drive type, as returned by
      GetDriveType()
   */

   UINT uDriveType;

   /* @field DWORD | dwSerialNumber | The volume's serial number. */

   DWORD dwSerialNumber;

   /*
      @field UINT | ucbVolumeLabelOffset | Offset in bytes of volume label
      string from base of structure.
   */

   UINT ucbVolumeLabelOffset;

   /*
      This member is for storing the unicode version of the string
   */

   UINT ucbVolumeLabelOffsetW;
}
IVOLUMEIDW;
DECLARE_STANDARD_TYPES(IVOLUMEIDW);
#endif

#ifdef UNICODE
#define IVOLUMEID   IVOLUMEIDW
#define PIVOLUMEID  PIVOLUMEIDW
#define CIVOLUMEID  CIVOLUMEIDW
#define PCIVOLUMEID PCIVOLUMEIDW
#else
#define IVOLUMEID   IVOLUMEIDA
#define PIVOLUMEID  PIVOLUMEIDA
#define CIVOLUMEID  CIVOLUMEIDA
#define PCIVOLUMEID PCIVOLUMEIDA
#endif

/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL UnifyIVolumeIDInfo(UINT, DWORD, LPCTSTR, PIVOLUMEID *, PUINT);
PRIVATE_CODE BOOL IsPathOnVolume(LPCTSTR, PCIVOLUMEID, PBOOL);
PRIVATE_CODE COMPARISONRESULT CompareUINTs(UINT, UINT);

#if defined(DEBUG) || defined (VSTF)

PRIVATE_CODE BOOL IsValidPCIVOLUMEID(PCIVOLUMEID);

#endif


/*
** UnifyIVolumeIDInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnifyIVolumeIDInfo(UINT uDriveType, DWORD dwSerialNumber,
                                LPCTSTR pcszVolumeLabel, PIVOLUMEID *ppivolid,
                                PUINT pucbIVolumeIDLen)
{
   BOOL bResult;
#ifdef UNICODE
   CHAR szAnsiVolumeLabel[MAX_PATH];
   BOOL bUnicode;
   UINT cchVolumeLabel;
   UINT cchChars;
#endif

   /* dwSerialNumber may be any value. */

   ASSERT(IsValidDriveType(uDriveType));
   ASSERT(IS_VALID_STRING_PTR(pcszVolumeLabel, CSTR));
   ASSERT(IS_VALID_WRITE_PTR(ppivolid, PIVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(pucbIVolumeIDLen, UINT));

   /* Assume we won't overflow *pucbIVolumeIDLen here. */

#ifdef UNICODE
   /* Determine whether we need a full fledged UNICODE volume ID */
   bUnicode = FALSE;
   cchVolumeLabel = WideCharToMultiByte(CP_ACP, 0,
                                        pcszVolumeLabel, -1,
                                        szAnsiVolumeLabel, MAX_PATH,
                                        0, 0);
   if ( cchVolumeLabel == 0 )
   {
      bUnicode = TRUE;
   }
   else
   {
      WCHAR szWideVolumeLabel[MAX_PATH];

      cchChars = MultiByteToWideChar(CP_ACP, 0,
                                     szAnsiVolumeLabel, -1,
                                     szWideVolumeLabel, MAX_PATH);
      if ( cchChars == 0 || lstrcmp(pcszVolumeLabel,szWideVolumeLabel) != 0 )
      {
         bUnicode = TRUE;
      }
   }

   if ( bUnicode )
   {
      UINT ucbDataSize;

      /* (+ 1) for null terminator. */

      ucbDataSize = SIZEOF(IVOLUMEIDW) + cchVolumeLabel;
      ucbDataSize = ALIGN_WORD_CNT(ucbDataSize);
      ucbDataSize += (lstrlen(pcszVolumeLabel) + 1) * SIZEOF(TCHAR);
      *pucbIVolumeIDLen = ucbDataSize;
   }
   else
   {
      /* (+ 1) for null terminator. */

      *pucbIVolumeIDLen = SIZEOF(IVOLUMEIDA) +
                          cchVolumeLabel;
   }
#else
   /* (+ 1) for null terminator. */

   *pucbIVolumeIDLen = SIZEOF(**ppivolid) +
                       (lstrlen(pcszVolumeLabel) + 1) * SIZEOF(TCHAR);
#endif

   bResult = AllocateMemory(*pucbIVolumeIDLen, ppivolid);

   if (bResult)
   {
      (*ppivolid)->ucbSize = *pucbIVolumeIDLen;
      (*ppivolid)->uDriveType = uDriveType;
      (*ppivolid)->dwSerialNumber = dwSerialNumber;

      /* Append volume label. */

#ifdef UNICODE
      if ( bUnicode )
      {
          (*ppivolid)->ucbVolumeLabelOffset = SIZEOF(IVOLUMEIDW);
          (*ppivolid)->ucbVolumeLabelOffsetW = ALIGN_WORD_CNT(
                                         SIZEOF(IVOLUMEIDW)+cchVolumeLabel);

          lstrcpy(IVOLID_Volume_Label_PtrW(*ppivolid), pcszVolumeLabel);
      }
      else
      {
          (*ppivolid)->ucbVolumeLabelOffset = SIZEOF(IVOLUMEIDA);
      }
      lstrcpyA(IVOLID_Volume_Label_PtrA(*ppivolid), szAnsiVolumeLabel);
#else

      lstrcpy(IVOLID_Volume_Label_Ptr(*ppivolid), pcszVolumeLabel);
#endif
   }

   ASSERT(! bResult ||
          (IS_VALID_STRUCT_PTR(*ppivolid, CIVOLUMEID) &&
           EVAL(*pucbIVolumeIDLen == GetVolumeIDLen((PCVOLUMEID)*ppivolid))));

   return(bResult);
}


/*
** IsPathOnVolume()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsPathOnVolume(LPCTSTR pcszDrivePath, PCIVOLUMEID pcivolid,
                                 PBOOL pbOnVolume)
{
   BOOL bResult;
   PVOLUMEID pvolid;
   UINT ucbVolumeIDLen;

   ASSERT(IsDrivePath(pcszDrivePath));
   ASSERT(IS_VALID_STRUCT_PTR(pcivolid, CIVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(pcivolid, CIVOLUMEID));

   bResult = CreateVolumeID(pcszDrivePath, &pvolid, &ucbVolumeIDLen);

   if (bResult)
   {
      *pbOnVolume = (CompareVolumeIDs(pvolid, (PCVOLUMEID)pcivolid)
                     == CR_EQUAL);

      DestroyVolumeID(pvolid);
   }

   return(bResult);
}


/*
** CompareUINTs()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT CompareUINTs(UINT uFirst, UINT uSecond)
{
   COMPARISONRESULT cr;

   /* Any UINTs are valid input. */

   if (uFirst < uSecond)
      cr = CR_FIRST_SMALLER;
   else if (uFirst > uSecond)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   ASSERT(IsValidCOMPARISONRESULT(cr));

   return(cr);
}


#if defined(DEBUG) || defined (VSTF)

/*
** IsValidPCIVOLUMEID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCIVOLUMEID(PCIVOLUMEID pcivolid)
{
   /* dwSerialNumber may be any value. */

   return(IS_VALID_READ_PTR(pcivolid, CIVOLUMEID) &&
          IS_VALID_READ_BUFFER_PTR(pcivolid, CIVOLUMEID, pcivolid->ucbSize) &&
          EVAL(IsValidDriveType(pcivolid->uDriveType)) &&
          EVAL(IsContained(pcivolid, pcivolid->ucbSize,
                           IVOLID_Volume_Label_Ptr(pcivolid),
                           lstrlen(IVOLID_Volume_Label_Ptr(pcivolid))*SIZEOF(TCHAR))));
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateVolumeID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateVolumeID(LPCTSTR pcszDrivePath, PVOLUMEID *ppvolid,
                                PUINT pucbVolumeIDLen)
{
   BOOL bResult;
   /* "C:\" + null terminator. */
   TCHAR rgchRootPath[3 + 1];
   TCHAR rgchVolumeLabel[MAX_PATH_LEN];
   DWORD dwSerialNumber;

   ASSERT(IsDrivePath(pcszDrivePath));
   ASSERT(IS_VALID_WRITE_PTR(ppvolid, PVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(pucbVolumeIDLen, UINT));

   /* Get volume's label and serial number. */

   MyLStrCpyN(rgchRootPath, pcszDrivePath, ARRAYSIZE(rgchRootPath));

   bResult = GetVolumeInformation(rgchRootPath, rgchVolumeLabel,
                                  ARRAYSIZE(rgchVolumeLabel), &dwSerialNumber,
                                  NULL, NULL, NULL, 0);

   if (bResult)
      /* Wrap them up. */
      bResult = UnifyIVolumeIDInfo(GetDriveType(rgchRootPath), dwSerialNumber,
                                   rgchVolumeLabel, (PIVOLUMEID *)ppvolid,
                                   pucbVolumeIDLen);

   ASSERT(! bResult ||
          IS_VALID_STRUCT_PTR((PCIVOLUMEID)*ppvolid, CIVOLUMEID));

   return(bResult);
}


/*
** DestroyVolumeID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyVolumeID(PVOLUMEID pvolid)
{
   ASSERT(IS_VALID_STRUCT_PTR(pvolid, CVOLUMEID));

   FreeMemory(pvolid);

   return;
}


/*
** CompareVolumeIDs()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Volume ID data is compared in the following order:
**    1) drive type
**    2) volume serial number
**
** N.b., volume labels are ignored.
*/
PUBLIC_CODE COMPARISONRESULT CompareVolumeIDs(PCVOLUMEID pcvolidFirst,
                                         PCVOLUMEID pcvolidSecond)
{
   COMPARISONRESULT cr;

   ASSERT(IS_VALID_STRUCT_PTR(pcvolidFirst, CVOLUMEID));
   ASSERT(IS_VALID_STRUCT_PTR(pcvolidSecond, CVOLUMEID));

   /* Compare VOLUMEIDs piece by piece. */

   cr = CompareUINTs(((PCIVOLUMEID)pcvolidFirst)->uDriveType,
                     ((PCIVOLUMEID)pcvolidSecond)->uDriveType);

   if (cr == CR_EQUAL)
      cr = CompareDWORDs(((PCIVOLUMEID)pcvolidFirst)->dwSerialNumber,
                         ((PCIVOLUMEID)pcvolidSecond)->dwSerialNumber);

   return(cr);
}


/*
** SearchForLocalPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL SearchForLocalPath(PCVOLUMEID pcvolid, LPCTSTR pcszFullPath,
                               DWORD dwInFlags, LPTSTR pszFoundPathBuf)
{
   BOOL bResult;
   BOOL bAvailable;
#if defined(DEBUG) && defined(UNICODE)
   WCHAR szWideVolumeLabel[MAX_PATH];
   LPWSTR pszWideVolumeLabel;
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));
   ASSERT(IsFullPath(pcszFullPath));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_SFLP_IFLAGS));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszFoundPathBuf, STR, MAX_PATH_LEN));

#if defined(DEBUG) && defined(UNICODE)
   if (((PCIVOLUMEID)pcvolid)->ucbVolumeLabelOffset == SIZEOF(IVOLUMEIDA))
   {
      pszWideVolumeLabel = szWideVolumeLabel;
      MultiByteToWideChar(CP_ACP, 0,
                          IVOLID_Volume_Label_PtrA((PCIVOLUMEID)pcvolid), -1,
                          szWideVolumeLabel, MAX_PATH);
   }
   else
   {
      pszWideVolumeLabel = IVOLID_Volume_Label_Ptr((PCIVOLUMEID)pcvolid);
   }
#endif

   /* Were we given a local path to check first? */

   if (IsLocalDrivePath(pcszFullPath))
      /* Yes.  Check it. */
      bResult = IsPathOnVolume(pcszFullPath, (PCIVOLUMEID)pcvolid,
                               &bAvailable);
   else
   {
      /* No. */

      bAvailable = FALSE;
      bResult = TRUE;
   }

   if (bResult)
   {
      /* Did we find the volume? */

      if (bAvailable)
      {
         /* Yes. */

         ASSERT(lstrlen(pcszFullPath) < MAX_PATH_LEN);
         lstrcpy(pszFoundPathBuf, pcszFullPath);
      }
      else
      {
         /*
          * No.  Should we search other matching local devices for the volume?
          */

         if (IS_FLAG_SET(dwInFlags, SFLP_IFL_LOCAL_SEARCH))
         {
            TCHAR chOriginalDrive;
            UINT uDrive;
            DWORD dwLogicalDrives;

            /* Yes. */

#ifdef UNICODE
            WARNING_OUT((TEXT("SearchForLocalPath(): Searching for local volume \"%s\", as requested."),
                         pszWideVolumeLabel));
#else
            WARNING_OUT((TEXT("SearchForLocalPath(): Searching for local volume \"%s\", as requested."),
                         IVOLID_Volume_Label_Ptr((PCIVOLUMEID)pcvolid)));
#endif

            ASSERT(IsCharAlpha(*pcszFullPath));
            chOriginalDrive = *pcszFullPath;

            ASSERT(lstrlen(pcszFullPath) < MAX_PATH_LEN);
            lstrcpy(pszFoundPathBuf, pcszFullPath);

            /* Get bit mask of local logical drives. */

            dwLogicalDrives = GetLogicalDrives();

            for (uDrive = 0; uDrive < MAX_LOCAL_DRIVES; uDrive++)
            {
               if (IS_FLAG_SET(dwLogicalDrives, (1 << uDrive)))
               {
                  TCHAR chDrive;

                  chDrive = (TCHAR)(TEXT('A') + uDrive);
                  ASSERT(IsCharAlpha(chDrive));

                  if (chDrive != chOriginalDrive)
                  {
                     TCHAR rgchLocalRootPath[DRIVE_ROOT_PATH_LEN];

                     lstrcpy(rgchLocalRootPath, TEXT("A:\\"));
                     rgchLocalRootPath[0] = chDrive;

                     /*
                      * Does this drive's type match the target volume's drive
                      * type?
                      */

                     if (GetDriveType(rgchLocalRootPath) == ((PCIVOLUMEID)pcvolid)->uDriveType)
                     {
                        /* Yes.  Check the volume. */

                        TRACE_OUT((TEXT("SearchForLocalPath(): Checking local root path %s."),
                                   rgchLocalRootPath));

                        bResult = IsPathOnVolume(rgchLocalRootPath,
                                                 (PCIVOLUMEID)pcvolid,
                                                 &bAvailable);

                        if (bResult)
                        {
                           if (bAvailable)
                           {
                              ASSERT(lstrlen(pcszFullPath) < MAX_PATH_LEN);
                              lstrcpy(pszFoundPathBuf, pcszFullPath);

                              ASSERT(IsCharAlpha(*pszFoundPathBuf));
                              *pszFoundPathBuf = chDrive;

                              TRACE_OUT((TEXT("SearchForLocalPath(): Found matching volume on local path %s."),
                                         pszFoundPathBuf));

                              break;
                           }
                        }
                        else
                           break;
                     }
                  }
               }
            }
         }
         else
            /* No. */
#ifdef UNICODE
            WARNING_OUT((TEXT("SearchForLocalPath(): Not searching for local volume \"%s\", as requested."),
                         pszWideVolumeLabel));
#else
            WARNING_OUT((TEXT("SearchForLocalPath(): Not searching for local volume \"%s\", as requested."),
                         IVOLID_Volume_Label_Ptr((PCIVOLUMEID)pcvolid)));
#endif
      }
   }

   ASSERT(! bResult ||
          ! bAvailable ||
          IsLocalDrivePath(pszFoundPathBuf));

   return(bResult && bAvailable);
}


/*
** GetVolumeIDLen()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE UINT GetVolumeIDLen(PCVOLUMEID pcvolid)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));

   return(((PCIVOLUMEID)pcvolid)->ucbSize);
}


/*
** GetVolumeSerialNumber()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetVolumeSerialNumber(PCVOLUMEID pcvolid,
                                  PCDWORD *ppcdwSerialNumber)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(ppcdwSerialNumber, PCDWORD));

   *ppcdwSerialNumber = &(((PCIVOLUMEID)pcvolid)->dwSerialNumber);

   ASSERT(IS_VALID_READ_PTR(*ppcdwSerialNumber, CDWORD));

   return(TRUE);
}


/*
** GetVolumeDriveType()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetVolumeDriveType(PCVOLUMEID pcvolid, PCUINT *ppcuDriveType)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(ppcuDriveType, PCUINT));

   *ppcuDriveType = &(((PCIVOLUMEID)pcvolid)->uDriveType);

   ASSERT(IS_VALID_READ_PTR(*ppcuDriveType, CUINT));

   return(TRUE);
}


/*
** GetVolumeLabel()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetVolumeLabel(PCVOLUMEID pcvolid, LPCSTR *ppcszVolumeLabel)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(ppcszVolumeLabel, LPCTSTR));

   *ppcszVolumeLabel = IVOLID_Volume_Label_PtrA((PCIVOLUMEID)pcvolid);

   ASSERT(IS_VALID_STRING_PTRA(*ppcszVolumeLabel, CSTR));

   return(TRUE);
}

#ifdef UNICODE
/*
** GetVolumeLabelW()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetVolumeLabelW(PCVOLUMEID pcvolid, LPCWSTR *ppcszVolumeLabel)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));
   ASSERT(IS_VALID_WRITE_PTR(ppcszVolumeLabel, LPCTSTR));

   if (((PCIVOLUMEID)pcvolid)->ucbVolumeLabelOffset == SIZEOF(IVOLUMEIDW))
   {
       *ppcszVolumeLabel = IVOLID_Volume_Label_PtrW((PCIVOLUMEID)pcvolid);

       ASSERT(IS_VALID_STRING_PTR(*ppcszVolumeLabel, CSTR));
   }
   else
   {
       *ppcszVolumeLabel = NULL;
   }

   return(TRUE);
}
#endif

/*
** CompareDWORDs()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT CompareDWORDs(DWORD dwFirst, DWORD dwSecond)
{
   COMPARISONRESULT cr;

   /* Any DWORDs are valid input. */

   if (dwFirst < dwSecond)
      cr = CR_FIRST_SMALLER;
   else if (dwFirst > dwSecond)
      cr = CR_FIRST_LARGER;
   else
      cr = CR_EQUAL;

   ASSERT(IsValidCOMPARISONRESULT(cr));

   return(cr);
}


#if defined(DEBUG) || defined (VSTF)

/*
** IsValidPCVOLUMEID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCVOLUMEID(PCVOLUMEID pcvolid)
{
   return(IS_VALID_STRUCT_PTR((PCIVOLUMEID)pcvolid, CIVOLUMEID));
}

#endif


#ifdef DEBUG

/*
** DumpVolumeID()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DumpVolumeID(PCVOLUMEID pcvolid)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcvolid, CVOLUMEID));

   PLAIN_TRACE_OUT((TEXT("%s%s[local volume ID] ucbSize = %#x"),
                    INDENT_STRING,
                    INDENT_STRING,
                    ((PCIVOLUMEID)pcvolid)->ucbSize));
   PLAIN_TRACE_OUT((TEXT("%s%s[local volume ID] drive type %u"),
                    INDENT_STRING,
                    INDENT_STRING,
                    ((PCIVOLUMEID)pcvolid)->uDriveType));
   PLAIN_TRACE_OUT((TEXT("%s%s[local volume ID] serial number %#08lx"),
                    INDENT_STRING,
                    INDENT_STRING,
                    ((PCIVOLUMEID)pcvolid)->dwSerialNumber));
   PLAIN_TRACE_OUT((TEXT("%s%s[local volume ID] label \"%s\""),
                    INDENT_STRING,
                    INDENT_STRING,
                    IVOLID_Volume_Label_Ptr((PCIVOLUMEID)pcvolid)));

   return;
}

#endif
