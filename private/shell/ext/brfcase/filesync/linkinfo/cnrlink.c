/*
 * cnrlink.c - CNRLink ADT module.
 */


/* Headers
 **********/

#include "project.h"
#pragma hdrstop

#include "cnrlink.h"
#include "server.h"


/* Constants
 ************/

/* WNetUseConnection() flag combinations */

#define ALL_CONNECT_IN_FLAGS     (CONNECT_UPDATE_PROFILE |\
                                  CONNECT_UPDATE_RECENT |\
                                  CONNECT_TEMPORARY |\
                                  CONNECT_INTERACTIVE |\
                                  CONNECT_PROMPT |\
                                  CONNECT_REDIRECT)

#define ALL_CONNECT_OUT_FLAGS    (CONNECT_REFCOUNT |\
                                  CONNECT_LOCALDRIVE)


/* Macros
 *********/

/* macros for accessing ICNRLINK data */

#define ICNRL_Remote_Name_PtrA(picnrl) \
   ((LPSTR)(((PBYTE)(picnrl)) + (picnrl)->ucbNetNameOffset))

#define ICNRL_Device_PtrA(picnrl) \
   ((LPSTR)(((PBYTE)(picnrl)) + (picnrl)->ucbDeviceOffset))

#define ICNRL_Remote_Name_PtrW(picnrl) \
   ((LPWSTR)(((PBYTE)(picnrl)) + (picnrl)->ucbNetNameOffsetW))

#define ICNRL_Device_PtrW(picnrl) \
   ((LPWSTR)(((PBYTE)(picnrl)) + (picnrl)->ucbDeviceOffsetW))

#define IS_ICNRL_ANSI(picnrl) \
   ((PBYTE)(picnrl) + ((PICNRLINKW)(picnrl))->ucbNetNameOffset) == (PBYTE)&(((PICNRLINKW)(picnrl))->ucbNetNameOffsetW)

#ifdef UNICODE
#define ICNRL_Remote_Name_Ptr(picnrl)   ICNRL_Remote_Name_PtrW(picnrl)
#define ICNRL_Device_Ptr(picnrl)        ICNRL_Device_PtrW(picnrl)
#else
#define ICNRL_Remote_Name_Ptr(picnrl)   ICNRL_Remote_Name_PtrA(picnrl)
#define ICNRL_Device_Ptr(picnrl)        ICNRL_Device_PtrA(picnrl)
#endif

/* Types
 ********/

/*
   @doc INTERNAL

   @enum ICNRLINKFLAGS | Internal CNRLink structure flags.
*/

typedef enum _icnrlinkflags
{
   /*
      @emem ICNRL_FL_VALID_DEVICE | If set, last redirected drive is valid.  If
      clear, last redirected drive is not valid.
   */

   ICNRL_FL_VALID_DEVICE = 0x0001,

   /*
      @emem ICNRL_FL_VALID_NET_TYPE | If set, net type is valid.  If clear, net
      type is not valid.
   */

   ICNRL_FL_VALID_NET_TYPE = 0x0002,

   /* @emem ALL_ICNRL_FLAGS | All internal CNRLink structure flags. */

   ALL_ICNRL_FLAGS = (ICNRL_FL_VALID_DEVICE |
                      ICNRL_FL_VALID_NET_TYPE)
}
ICNRLINKFLAGS;

/*
   @doc INTERNAL

   @struct ICNRLINK | Internal definition of relocatable connectable network
   resource (CNR) link structure.  An <t ILINKINFO> structure may contain an
   ICNRLINK structure.  An ICNRLINK structure consists of a header described as
   below, followed by variable-length data.
*/

typedef struct _icnrlinkA
{
   /*
      @field UINT | ucbSize | Length of ICNRLINK structure in bytes, including
      ucbSize field.
   */

   UINT ucbSize;

   /*
      @field DWORD | dwFlags | A bit mask of flags from the <t ICNRLINKFLAGS>
      enumeration.
   */

   DWORD dwFlags;

   /*
      @field UINT | ucbNetNameOffset | Offset in bytes of CNR name string from
      base of structure.  The CNR name string may be passed to
      WNetUseConnection() to add a connection to the CNR.<nl>
      Example CNRLink name string: "\\\\fredbird\\work".
   */

   UINT ucbNetNameOffset;

   /*
      @field UINT | ucbDeviceOffset | Offset in bytes of last redirected local
      device string from base of structure.  This field is only valid if
      ICNRL_FL_VALID_DEVICE is set in dwFlags.  The last redirected local
      device string may be passed to WNetUseConnection() to add a redirected
      device connection to the CNR.<nl>
      Example last redirected local device string: "D:".
   */

   UINT ucbDeviceOffset;

   /*
      @field DWORD | dwNetType | The network type as returned in a
      NETINFOSTRUCT.  This field is only valid if ICNRL_FL_VALID_NET_TYPE is
      set in dwFlags.  The net type is used to retrieve the host net resource's
      host NP's name to use in calling WNetUseConnection().<nl>
      Example net type: WNNC_NET_NETWARE.
   */

   DWORD dwNetType;
}
ICNRLINKA;
DECLARE_STANDARD_TYPES(ICNRLINKA);

#ifdef UNICODE
typedef struct _icnrlinkW
{
   /*
      @field UINT | ucbSize | Length of ICNRLINK structure in bytes, including
      ucbSize field.
   */

   UINT ucbSize;

   /*
      @field DWORD | dwFlags | A bit mask of flags from the <t ICNRLINKFLAGS>
      enumeration.
   */

   DWORD dwFlags;

   /*
      @field UINT | ucbNetNameOffset | Offset in bytes of CNR name string from
      base of structure.  The CNR name string may be passed to
      WNetUseConnection() to add a connection to the CNR.<nl>
      Example CNRLink name string: "\\\\fredbird\\work".
   */

   UINT ucbNetNameOffset;

   /*
      @field UINT | ucbDeviceOffset | Offset in bytes of last redirected local
      device string from base of structure.  This field is only valid if
      ICNRL_FL_VALID_DEVICE is set in dwFlags.  The last redirected local
      device string may be passed to WNetUseConnection() to add a redirected
      device connection to the CNR.<nl>
      Example last redirected local device string: "D:".
   */

   UINT ucbDeviceOffset;

   /*
      @field DWORD | dwNetType | The network type as returned in a
      NETINFOSTRUCT.  This field is only valid if ICNRL_FL_VALID_NET_TYPE is
      set in dwFlags.  The net type is used to retrieve the host net resource's
      host NP's name to use in calling WNetUseConnection().<nl>
      Example net type: WNNC_NET_NETWARE.
   */

   DWORD dwNetType;

   /*
      These members are for storing the unicode version of the strings
   */
   UINT ucbNetNameOffsetW;
   UINT ucbDeviceOffsetW;
}
ICNRLINKW;
DECLARE_STANDARD_TYPES(ICNRLINKW);
#endif

#ifdef UNICODE
#define ICNRLINK    ICNRLINKW
#define PICNRLINK   PICNRLINKW
#define CICNRLINK   CICNRLINKW
#define PCICNRLINK  PCICNRLINKW
#else
#define ICNRLINK    ICNRLINKA
#define PICNRLINK   PICNRLINKA
#define CICNRLINK   CICNRLINKA
#define PCICNRLINK  PCICNRLINKA
#endif

/* Exported from MPR.DLL, but not in winnetwk.h
*/
#ifdef UNICODE
DWORD APIENTRY WNetGetResourceInformationW (LPNETRESOURCE lpNetResource, LPVOID lpBuffer, LPDWORD cbBuffer, LPTSTR * lplpSystem);
#define WNetGetResourceInformation WNetGetResourceInformationW
#else
DWORD APIENTRY WNetGetResourceInformationA (LPNETRESOURCE lpNetResource, LPVOID lpBuffer, LPDWORD cbBuffer, LPTSTR * lplpSystem);
#define WNetGetResourceInformation WNetGetResourceInformationA
#endif


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE BOOL GetNetPathFromLocalPath(LPCTSTR, LPTSTR, LPCTSTR *, PBOOL, PDWORD);
PRIVATE_CODE BOOL UnifyICNRLinkInfo(LPCTSTR, DWORD, LPCTSTR, DWORD, PICNRLINK *, PUINT);
PRIVATE_CODE BOOL GetNetType(LPCTSTR, PDWORD);
PRIVATE_CODE BOOL GetNetProviderName(PCICNRLINK, LPTSTR);
PRIVATE_CODE COMPARISONRESULT CompareNetNames(LPCTSTR, LPCTSTR);
PRIVATE_CODE BOOL SearchForRedirectedConnection(PCICNRLINK, LPTSTR);

#if defined(DEBUG) || defined (VSTF)

PRIVATE_CODE BOOL IsValidDevice(LPCTSTR);
PRIVATE_CODE BOOL IsValidNetType(DWORD);
PRIVATE_CODE BOOL IsValidPCICNRLINK(PCICNRLINK);

#endif

#if defined(DEBUG)

PRIVATE_CODE BOOL IsValidNetProviderName(LPCTSTR);

#endif

#if 0 //#ifndef BUGBUG_JONBE

DWORD APIENTRY
WNetGetNetworkInformationW(
    LPCWSTR          lpProvider,
    LPNETINFOSTRUCT   lpNetInfoStruct
    )
{
#if 0
    if (wcsicmp(lpProvider, L"Microsoft Windows Network") == 0)
    {
        lpNetInfoStruct->wNetType = (WORD)WNNC_NET_LANMAN;
        return ERROR_SUCCESS;
    }
    else if (wcsicmp(lpProvider, L"Novell Network") == 0)
    {
        lpNetInfoStruct->wNetType = (WORD)WNNC_NET_NETWARE;
        return ERROR_SUCCESS;
    }
    else
#endif
    {
        return ERROR_NOT_SUPPORTED;
    }
}
#endif

/*
** GetNetPathFromLocalPath()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetNetPathFromLocalPath(LPCTSTR pcszLocalPath,
                                          LPTSTR pszNetNameBuf,
                                          LPCTSTR *ppcszCommonPathSuffix,
                                          PBOOL pbIsShared, PDWORD pdwNetType)
{
   BOOL bResult = TRUE;
   PCSERVERVTABLE pcsvt;

   ASSERT(IsDrivePath(pcszLocalPath));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszNetNameBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(ppcszCommonPathSuffix, LPCTSTR));
   ASSERT(IS_VALID_WRITE_PTR(pbIsShared, BOOL));
   ASSERT(IS_VALID_WRITE_PTR(pdwNetType, DWORD));

   *pbIsShared = FALSE;

   if (GetServerVTable(&pcsvt))
   {
      TCHAR rgchSharedPath[MAX_PATH_LEN];

      ASSERT(lstrlen(pcszLocalPath) < ARRAYSIZE(rgchSharedPath));
      lstrcpy(rgchSharedPath, pcszLocalPath);

      FOREVER
      {
         if ((pcsvt->GetNetResourceFromLocalPath)(rgchSharedPath,
                                                  pszNetNameBuf, MAX_PATH_LEN,
                                                  pdwNetType))
         {
            ASSERT(lstrlen(pszNetNameBuf) < MAX_PATH_LEN);

            /* Determine common path suffix. */

            *ppcszCommonPathSuffix = pcszLocalPath + lstrlen(rgchSharedPath);

            /* Skip any leading slash. */

            if (IS_SLASH(**ppcszCommonPathSuffix))
               *ppcszCommonPathSuffix = CharNext(*ppcszCommonPathSuffix);

            ASSERT(! IS_SLASH(**ppcszCommonPathSuffix));

            // if it is terminated with a $ it is a hidden share, in that
            // case don't consider this shared
            *pbIsShared = pszNetNameBuf[lstrlen(pszNetNameBuf) -1] != TEXT('$');

            break;
         }
         else
         {
            if (! DeleteLastDrivePathElement(rgchSharedPath))
               break;
         }
      }
   }

   ASSERT(! bResult ||
          ! *pbIsShared ||
          (EVAL(IsUNCPath(pszNetNameBuf)) &&
           IS_VALID_STRING_PTR(*ppcszCommonPathSuffix, CSTR) &&
           EVAL(*ppcszCommonPathSuffix >= pcszLocalPath) &&
           EVAL(IsStringContained(pcszLocalPath, *ppcszCommonPathSuffix)) &&
           EVAL(IsValidNetType(*pdwNetType))));

   return(bResult);
}


/*
** UnifyICNRLinkInfo()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnifyICNRLinkInfo(LPCTSTR pcszNetName, DWORD dwFlags,
                                    LPCTSTR pcszDevice, DWORD dwNetType,
                                    PICNRLINK *ppicnrl, PUINT pucbICNRLinkLen)
{
   BOOL bResult;
   UINT ucbDataOffset;
#ifdef UNICODE
   BOOL bUnicode;
   UINT cchChars;
   CHAR szAnsiNetName[MAX_PATH];
   CHAR szAnsiDevice[MAX_PATH];
   UINT cbAnsiNetName;
   UINT cbWideNetName;
   UINT cbAnsiDevice;
   UINT cbWideDevice;
   UINT cbChars;
#endif

   ASSERT(IsUNCPath(pcszNetName));
   ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_ICNRL_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwFlags, ICNRL_FL_VALID_DEVICE) ||
          IsValidDevice(pcszDevice));
   ASSERT(IS_FLAG_CLEAR(dwFlags, ICNRL_FL_VALID_NET_TYPE) ||
          IsValidNetType(dwNetType));
   ASSERT(IS_VALID_WRITE_PTR(ppicnrl, PCNRLINK));
   ASSERT(IS_VALID_WRITE_PTR(pucbICNRLinkLen, UINT));

#ifdef UNICODE
   bUnicode = FALSE;

   cbAnsiNetName = WideCharToMultiByte(CP_ACP, 0,
                                       pcszNetName, -1,
                                       szAnsiNetName, MAX_PATH,
                                       0, 0);
   if ( cbAnsiNetName == 0 )
   {
      bUnicode = FALSE;
   }
   else
   {
      WCHAR szWideNetName[MAX_PATH];

      cbChars = MultiByteToWideChar(CP_ACP, 0,
                                    szAnsiNetName, -1,
                                    szWideNetName, MAX_PATH);
      if ( cbChars == 0 || lstrcmp(pcszNetName,szWideNetName) != 0 )
      {
         bUnicode = TRUE;
      }
   }

   if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_DEVICE))
   {
      cbAnsiDevice = WideCharToMultiByte(CP_ACP, 0,
                                         pcszDevice, -1,
                                         szAnsiDevice, MAX_PATH,
                                         0, 0);
      if ( cbAnsiDevice == 0 )
      {
         bUnicode = FALSE;
      }
      else
      {
         WCHAR szWideDevice[MAX_PATH];

         cchChars = MultiByteToWideChar(CP_ACP, 0,
                                        szAnsiDevice, -1,
                                        szWideDevice, MAX_PATH);
         if ( cchChars == 0 || lstrcmp(pcszDevice,szWideDevice) != 0 )
         {
            bUnicode = TRUE;
         }
      }
   }
   else
   {
      cbAnsiDevice = 0;
   }

   if ( bUnicode )
   {
      ucbDataOffset = SIZEOF(ICNRLINKW);

      /* (+ 1) for null terminator. */
      cbWideNetName = (lstrlen(pcszNetName) + 1) * sizeof(TCHAR);

      if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_DEVICE))
         cbWideDevice = (lstrlen(pcszDevice) + 1) * sizeof(TCHAR);
      else
         cbWideDevice = 0;

   }
   else
   {
      ucbDataOffset = SIZEOF(ICNRLINKA);

      cbWideNetName = 0;
      cbWideDevice  = 0;
   }

   *pucbICNRLinkLen = ucbDataOffset +
                      cbAnsiNetName +
                      cbAnsiDevice;
   if ( bUnicode )
   {
       *pucbICNRLinkLen = ALIGN_WORD_CNT(*pucbICNRLinkLen) +
                          cbWideNetName +
                          cbWideDevice;
   }

#else

   /* Assume we won't overflow *pucbICNRLinkLen here. */

   /* (+ 1) for null terminator. */

   *pucbICNRLinkLen = SIZEOF(**ppicnrl) +
                      (lstrlen(pcszNetName) + 1) * SIZEOF(TCHAR);

   if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_DEVICE))
      /* (+ 1) for null terminator. */
      *pucbICNRLinkLen += (lstrlen(pcszDevice) + 1) * SIZEOF(TCHAR);

   ucbDataOffset = SIZEOF(ICNRLINKA);
#endif

   bResult = AllocateMemory(*pucbICNRLinkLen, ppicnrl);

   if (bResult)
   {
      (*ppicnrl)->ucbSize = *pucbICNRLinkLen;
      (*ppicnrl)->dwFlags = dwFlags;

      if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_NET_TYPE))
         (*ppicnrl)->dwNetType = dwNetType;
      else
         (*ppicnrl)->dwNetType = 0;

      /* Append remote name. */

      (*ppicnrl)->ucbNetNameOffset = ucbDataOffset;

#ifdef UNICODE
      lstrcpyA(ICNRL_Remote_Name_PtrA(*ppicnrl), szAnsiNetName);
      ucbDataOffset += cbAnsiNetName;

      if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_DEVICE))
      {
         /* Append device name. */

         (*ppicnrl)->ucbDeviceOffset = ucbDataOffset;
         lstrcpyA(ICNRL_Device_PtrA(*ppicnrl), szAnsiDevice);

         ucbDataOffset += cbAnsiDevice;
      }
      else
      {
         (*ppicnrl)->ucbDeviceOffset = 0;
      }

      if ( bUnicode )
      {
         ucbDataOffset = ALIGN_WORD_CNT(ucbDataOffset);

         (*ppicnrl)->ucbNetNameOffsetW = ucbDataOffset;

         lstrcpy(ICNRL_Remote_Name_PtrW(*ppicnrl), pcszNetName);
         ucbDataOffset += cbWideNetName;

         if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_DEVICE))
         {
            /* Append device name. */

            (*ppicnrl)->ucbDeviceOffsetW = ucbDataOffset;
            lstrcpy(ICNRL_Device_Ptr(*ppicnrl), pcszDevice);

            /* (+ 1) for null terminator. */
            ucbDataOffset += cbWideDevice;
         }
         else
         {
            (*ppicnrl)->ucbDeviceOffsetW = 0;
         }

      }
#else
      lstrcpy(ICNRL_Remote_Name_Ptr(*ppicnrl), pcszNetName);
      /* (+ 1) for null terminator. */
      ucbDataOffset += lstrlen(pcszNetName) + 1;

      if (IS_FLAG_SET(dwFlags, ICNRL_FL_VALID_DEVICE))
      {
         /* Append device name. */

         (*ppicnrl)->ucbDeviceOffset = ucbDataOffset;
         lstrcpy(ICNRL_Device_Ptr(*ppicnrl), pcszDevice);
#ifdef DEBUG
         /* (+ 1) for null terminator. */
         ucbDataOffset += (lstrlen(pcszDevice) + 1) * SIZEOF(TCHAR);
#endif
      }
      else
         (*ppicnrl)->ucbDeviceOffset = 0;
#endif

      /* Do all the calculated lengths match? */

      ASSERT(ucbDataOffset == (*ppicnrl)->ucbSize);
      ASSERT(ucbDataOffset == *pucbICNRLinkLen);
   }

   ASSERT(! bResult ||
          (IS_VALID_STRUCT_PTR(*ppicnrl, CICNRLINK) &&
           EVAL(*pucbICNRLinkLen == GetCNRLinkLen((PCCNRLINK)*ppicnrl))));

   return(bResult);
}


/*
** GetNetType()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetNetType(LPCTSTR pcszCNRName, PDWORD pdwNetType)
{
   BOOL bResult = FALSE;
   NETRESOURCE nrIn;
   NETRESOURCEBUF nrbufOut;
   DWORD dwcbBufLen = SIZEOF(nrbufOut);
   LPTSTR pszFileSysPath;
   DWORD dwNetResult;
#ifdef DEBUG
   DWORD dwcmsTicks;
#endif

   ASSERT(IsValidCNRName(pcszCNRName));
   ASSERT(IS_VALID_WRITE_PTR(pdwNetType, DWORD));

   /* RAIDRAID: (15691) We only support disk resource connections here. */

   ZeroMemory(&nrIn, SIZEOF(nrIn));
   nrIn.lpRemoteName = (LPTSTR)pcszCNRName;
   nrIn.dwType = RESOURCETYPE_DISK;

#ifdef DEBUG
   dwcmsTicks = GetTickCount();
#endif

   dwNetResult = WNetGetResourceInformation(&nrIn, &(nrbufOut.rgbyte),
                                            &dwcbBufLen, &pszFileSysPath);

#ifdef DEBUG

   dwcmsTicks = GetTickCount() - dwcmsTicks;

   TRACE_OUT((TEXT("GetRemotePathInfo(): WNetGetResourceInformation() on net resource %s took %lu.%03lu seconds."),
              pcszCNRName,
              (dwcmsTicks / 1000),
              (dwcmsTicks % 1000)));

#endif

   if (dwNetResult == ERROR_SUCCESS)
   {
      if (nrbufOut.nr.lpProvider)
      {
         NETINFOSTRUCT nis;

         ASSERT(IS_VALID_STRING_PTR(nrbufOut.nr.lpProvider, STR));

         nis.cbStructure = SIZEOF(nis);

         dwNetResult = WNetGetNetworkInformation(nrbufOut.nr.lpProvider, &nis);

         if (dwNetResult == ERROR_SUCCESS)
         {
            *pdwNetType = ((nis.wNetType) << 16);
            bResult = TRUE;

            TRACE_OUT((TEXT("GetNetType(): Net type for CNR %s is %#08lx."),
                       pcszCNRName,
                       *pdwNetType));
         }
         else
            WARNING_OUT((TEXT("GetNetType(): WNetGetNetworkInformation() failed for %s NP, returning %lu."),
                         nrbufOut.nr.lpProvider,
                         dwNetResult));
      }
      else
         WARNING_OUT((TEXT("GetNetType(): WNetGetResourceInformation() was unable to determine the NP for CNR %s."),
                      pcszCNRName));
   }
   else
      WARNING_OUT((TEXT("GetNetType(): WNetGetResourceInformation() failed for CNR %s, returning %lu."),
                   pcszCNRName,
                   dwNetResult));

   ASSERT(! bResult ||
          IsValidNetType(*pdwNetType));

   return(bResult);
}


/*
** GetNetProviderName()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL GetNetProviderName(PCICNRLINK pcicnrl, LPTSTR pszNPNameBuf)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(pcicnrl, CICNRLINK));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszNPNameBuf, STR, MAX_PATH_LEN));

   if (IS_FLAG_SET(pcicnrl->dwFlags, ICNRL_FL_VALID_NET_TYPE))
   {
      DWORD dwcbNPNameBufLen;
      DWORD dwNetResult;

      dwcbNPNameBufLen = MAX_PATH_LEN;

      dwNetResult = WNetGetProviderName(pcicnrl->dwNetType, pszNPNameBuf,
                                        &dwcbNPNameBufLen);

      if (dwNetResult == ERROR_SUCCESS)
      {
         bResult = TRUE;

#ifdef UNICODE
         //
         // Unicode builds need to accept both ansi and unicode ICNRLINK structures.
         // Note the use of '%S' (upper case).  This will accept an ANSI string
         // in a UNICODE build environment.
         //
         if (IS_ICNRL_ANSI(pcicnrl))
             TRACE_OUT((TEXT("GetNetProviderName(): NP for CNR %S is %s."),
                        ICNRL_Remote_Name_PtrA(pcicnrl),
                        pszNPNameBuf));
         else
#endif
             TRACE_OUT((TEXT("GetNetProviderName(): NP for CNR %s is %s."),
                        ICNRL_Remote_Name_Ptr(pcicnrl),                       
                        pszNPNameBuf));
      }
      else
         WARNING_OUT((TEXT("GetNetProviderName(): WNetGetProviderName() failed for CNR %s's net type %#08lx, returning %lu."),
                      TEXT("<Remote Name>"), // ICNRL_Remote_Name_Ptr(pcicnrl),
                      pcicnrl->dwNetType,
                      dwNetResult));
   }
   else
      WARNING_OUT((TEXT("GetNetProviderName(): Net type for CNR %s is not known.  Unable to determine NP name."),
                   TEXT("<Remote Name>"))); // ICNRL_Remote_Name_Ptr(pcicnrl)));

   ASSERT(! bResult ||
          IsValidNetProviderName(pszNPNameBuf));

   return(bResult);
}


/*
** CompareNetNames()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT CompareNetNames(LPCTSTR pcszFirstNetName,
                                              LPCTSTR pcszSecondNetName)
{
   ASSERT(IS_VALID_STRING_PTR(pcszFirstNetName, CSTR));
   ASSERT(IS_VALID_STRING_PTR(pcszSecondNetName, CSTR));

   return(MapIntToComparisonResult(lstrcmp(pcszFirstNetName,
                                           pcszSecondNetName)));
}


/*
** SearchForRedirectedConnection()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL SearchForRedirectedConnection(PCICNRLINK pcicnrl,
                                                LPTSTR pszRootPathBuf)
{
   BOOL bResult = FALSE;
   HANDLE henum;
   DWORD dwNetResult;

   ASSERT(IS_VALID_STRUCT_PTR(pcicnrl, CICNRLINK));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));

#ifdef DEBUG
#ifdef UNICODE
   {
       LPWSTR pszWideNetName;
       WCHAR szWideNetName[MAX_PATH];

       if (IS_ICNRL_ANSI(pcicnrl))
       {
           pszWideNetName = szWideNetName;

           MultiByteToWideChar(CP_ACP, 0,
                               ICNRL_Remote_Name_PtrA(pcicnrl), -1,
                               szWideNetName, MAX_PATH);
       } else {
           pszWideNetName = ICNRL_Remote_Name_PtrW(pcicnrl);
       }

       WARNING_OUT((TEXT("SearchForRedirectedConnection(): Enumerating local connections searching for redirected connection to CNR \"%s\"."),
                pszWideNetName));
    }
#else
    WARNING_OUT((TEXT("SearchForRedirectedConnection(): Enumerating local connections searching for redirected connection to CNR \"%s\"."),
                ICNRL_Remote_Name_Ptr(pcicnrl)));
#endif
#endif

   /* RAIDRAID: (15691) We only support container resources here. */

   dwNetResult = WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK,
                              RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED,
                              NULL, &henum);

   if (dwNetResult == WN_SUCCESS)
   {
      DWORD dwc = 1;
      NETRESOURCEBUF nrbuf;
      DWORD dwcbBufLen = SIZEOF(nrbuf);

      while ((dwNetResult = WNetEnumResource(henum, &dwc, &(nrbuf.rgbyte),
                                             &dwcbBufLen))
             == WN_SUCCESS)
      {
         /* Is this a redirected connection? */

         if (nrbuf.nr.lpRemoteName != NULL)
         {
            if (nrbuf.nr.lpLocalName != NULL)
            {
               /* Yes.  Is it a redirected connection to the desired CNR? */

#ifdef UNICODE
               WCHAR szWideNetName[MAX_PATH];
               LPWSTR pszWideNetName;

               if (IS_ICNRL_ANSI(pcicnrl))
               {
                  pszWideNetName = szWideNetName;
                  MultiByteToWideChar(CP_ACP, 0,
                                      ICNRL_Remote_Name_PtrA(pcicnrl), -1,
                                      szWideNetName, MAX_PATH);
               }
               else
               {
                  pszWideNetName = ICNRL_Remote_Name_Ptr(pcicnrl);
               }
               if (CompareNetNames(pszWideNetName,
                                   nrbuf.nr.lpRemoteName)
                   == CR_EQUAL)
#else
               if (CompareNetNames(ICNRL_Remote_Name_Ptr(pcicnrl),
                                   nrbuf.nr.lpRemoteName)
                   == CR_EQUAL)
#endif
               {
                  /* Yes. */

                  ASSERT(lstrlen(nrbuf.nr.lpLocalName) < MAX_PATH_LEN);

                  lstrcpy(pszRootPathBuf, nrbuf.nr.lpLocalName);
                  bResult = TRUE;

                  TRACE_OUT((TEXT("SearchForRedirectedConnection(): Found CNR \"%s\" connected to %s."),
                             nrbuf.nr.lpRemoteName,
                             pszRootPathBuf));

                  break;
               }
               else
                  /* No. */
                  TRACE_OUT((TEXT("SearchForRedirectedConnection(): Skipping unmatched enumerated connection to CNR \"%s\" on %s."),
                             nrbuf.nr.lpRemoteName,
                             nrbuf.nr.lpLocalName));
            }
            else
               /* No. */
               TRACE_OUT((TEXT("SearchForRedirectedConnection(): Skipping enumerated deviceless connection to CNR \"%s\"."),
                          nrbuf.nr.lpRemoteName));
         }
         else
            WARNING_OUT((TEXT("SearchForRedirectedConnection(): Skipping enumerated connection with no CNR name.")));
      }

      if (! bResult && dwNetResult != WN_NO_MORE_ENTRIES)
         WARNING_OUT((TEXT("SearchForRedirectedConnection(): WNetEnumResource() failed, returning %lu."),
                      dwNetResult));

      dwNetResult = WNetCloseEnum(henum);

      if (dwNetResult != WN_SUCCESS)
         WARNING_OUT((TEXT("SearchForRedirectedConnection(): WNetCloseEnum() failed, returning %lu."),
                      dwNetResult));
   }
   else
      WARNING_OUT((TEXT("SearchForRedirectedConnection(): WNetOpenEnum() failed, returning %lu."),
                   dwNetResult));

   return(bResult);
}


#if defined(DEBUG) || defined (VSTF)

/*
** IsValidDevice()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidDevice(LPCTSTR pcszDevice)
{
   /* Any valid string < MAX_PATH_LEN bytes long is a valid device name. */

   return(IS_VALID_STRING_PTR(pcszDevice, CSTR) &&
          EVAL(lstrlen(pcszDevice) < MAX_PATH_LEN));
}


/*
** IsValidNetType()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidNetType(DWORD dwNetType)
{
   BOOL bResult;

   switch (dwNetType & 0xffff0000)
   {
      case WNNC_NET_MSNET:
      case WNNC_NET_VINES:
      case WNNC_NET_10NET:
      case WNNC_NET_LOCUS:
      case WNNC_NET_SUN_PC_NFS:
      case WNNC_NET_LANSTEP:
      case WNNC_NET_9TILES:
      case WNNC_NET_LANTASTIC:
      case WNNC_NET_AS400:
      case WNNC_NET_FTP_NFS:
      case WNNC_NET_PATHWORKS:
      case WNNC_NET_LIFENET:
      case WNNC_NET_POWERLAN:
      case WNNC_NET_BWNFS:
      case WNNC_NET_COGENT:
      case WNNC_NET_FARALLON:
      case WNNC_NET_APPLETALK:
         WARNING_OUT((TEXT("IsValidNetType(): Unexpected net type %#08lx is neither NetWare nor LANMan."),
                      dwNetType));
         /* Fall through... */

      case WNNC_NET_LANMAN:
      case WNNC_NET_NETWARE:
         bResult = TRUE;
         break;

      default:
         bResult = FALSE;
         ERROR_OUT((TEXT("IsValidNetType(): Invalid net type %#08lx."),
                    dwNetType));
         break;
   }

   if (dwNetType & 0x0000ffff)
      WARNING_OUT((TEXT("IsValidNetType(): Low word of net type %#08lx is non-zero."),
                   dwNetType));

   return(bResult);
}


/*
** IsValidPCICNRLINK()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCICNRLINK(PCICNRLINK pcicnrl)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pcicnrl, CICNRLINK) &&
       IS_VALID_READ_BUFFER_PTR(pcicnrl, CICNRLINK, pcicnrl->ucbSize) &&
       FLAGS_ARE_VALID(pcicnrl->dwFlags, ALL_ICNRL_FLAGS) &&
       EVAL(IsValidCNRName(ICNRL_Remote_Name_Ptr(pcicnrl))) &&
       EVAL(IsContained(pcicnrl, pcicnrl->ucbSize,
                        ICNRL_Remote_Name_PtrA(pcicnrl),
                        lstrlenA(ICNRL_Remote_Name_PtrA(pcicnrl)))) &&
       (IS_FLAG_CLEAR(pcicnrl->dwFlags, ICNRL_FL_VALID_NET_TYPE) ||
        EVAL(IsValidNetType(pcicnrl->dwNetType))))
   {
      if (IS_FLAG_CLEAR(pcicnrl->dwFlags, ICNRL_FL_VALID_DEVICE))
      {
         ASSERT(! pcicnrl->ucbDeviceOffset);
         bResult = TRUE;
      }
      else
         bResult = (EVAL(IsValidDevice(ICNRL_Device_Ptr(pcicnrl))) &&
                    EVAL(IsContained(pcicnrl, pcicnrl->ucbSize,
                                     ICNRL_Device_PtrA(pcicnrl),
                                     lstrlenA(ICNRL_Device_PtrA(pcicnrl)))));
   }
   else
      bResult = FALSE;

   return(bResult);
}

#endif


#if defined(DEBUG)

/*
** IsValidNetProviderName()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidNetProviderName(LPCTSTR pcszNetProvider)
{
   /* Any string < MAX_PATH_LEN characters long is a valid NP name. */

   return(IS_VALID_STRING_PTR(pcszNetProvider, CSTR) &&
          lstrlen(pcszNetProvider) < MAX_PATH_LEN);
}

#endif


/****************************** Public Functions *****************************/


/*
** CreateLocalCNRLink()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** If TRUE is returned:
**    1) *ppcnrl is only valid if *pucbCNRLinkLen > 0.
**    2) pszLocalBasePathBuf is valid.
**    3) *ppcszCommonPathSuffix is valid.
**
** If *pucbCNRLinkLen == 0, pszLocalBasePathBuf is a copy of pcszLocalPath, and
** *ppcszCommonPathSuffix points at the null terminator of pcszLocalPath.
**
** If *pucbCNRLinkLen > 0, pszLocalBasePathBuf is the closest shared local base
** path, and *ppcszCommonPathSuffix points at that path's suffix in
** pcszLocalPath.
*/
PUBLIC_CODE BOOL CreateLocalCNRLink(LPCTSTR pcszLocalPath, PCNRLINK *ppcnrl,
                                    PUINT pucbCNRLinkLen,
                                    LPTSTR pszLocalBasePathBuf,
                                    LPCTSTR *ppcszCommonPathSuffix)
{
   BOOL bResult;
   TCHAR rgchNetName[MAX_PATH_LEN];
   BOOL bShared;
   DWORD dwNetType;

   ASSERT(IsDrivePath(pcszLocalPath));
   ASSERT(IS_VALID_WRITE_PTR(ppcnrl, PCNRLINK));
   ASSERT(IS_VALID_WRITE_PTR(pucbCNRLinkLen, UINT));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszLocalBasePathBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(ppcszCommonPathSuffix, LPCTSTR));

   bResult = GetNetPathFromLocalPath(pcszLocalPath, rgchNetName,
                                     ppcszCommonPathSuffix, &bShared,
                                     &dwNetType);

   if (bResult)
   {
      if (bShared)
      {
         bResult = UnifyICNRLinkInfo(rgchNetName, ICNRL_FL_VALID_NET_TYPE,
                                     NULL, dwNetType, (PICNRLINK *)ppcnrl,
                                     pucbCNRLinkLen);

         if (bResult)
         {
            UINT ucbLocalBasePathLen;

            /* Copy local base path into output buffer. */

            ASSERT(*ppcszCommonPathSuffix >= pcszLocalPath);
            ucbLocalBasePathLen = (UINT)(*ppcszCommonPathSuffix - pcszLocalPath);

            CopyMemory(pszLocalBasePathBuf, pcszLocalPath, ucbLocalBasePathLen * sizeof(TCHAR));
            pszLocalBasePathBuf[ucbLocalBasePathLen] = TEXT('\0');
         }
      }
      else
      {
         /* Not shared.  No CNRLink. */

         *pucbCNRLinkLen = 0;

         /* Copy entire local path into output buffer. */

         lstrcpy(pszLocalBasePathBuf, pcszLocalPath);

         /* Common path suffix is the empty string. */

         *ppcszCommonPathSuffix = pcszLocalPath + lstrlen(pcszLocalPath);
      }
   }

   ASSERT(! bResult ||
          (EVAL(IsDrivePath(pszLocalBasePathBuf)) &&
           IS_VALID_STRING_PTR(*ppcszCommonPathSuffix, CSTR) &&
           EVAL(IsStringContained(pcszLocalPath, *ppcszCommonPathSuffix)) &&
           (! *pucbCNRLinkLen ||
            (IS_VALID_STRUCT_PTR((PCICNRLINK)*ppcnrl, CICNRLINK) &&
             EVAL(*pucbCNRLinkLen == GetCNRLinkLen(*ppcnrl))))));

   return(bResult);
}


/*
** CreateRemoteCNRLink()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateRemoteCNRLink(LPCTSTR pcszRemotePath, LPCTSTR pcszCNRName,
                                     PCNRLINK *ppcnrl, PUINT pucbCNRLinkLen)
{
   BOOL bResult;
   /* "D:" + null terminator. */
   TCHAR rgchDrive[3];
   DWORD dwNetType;

   ASSERT(IsCanonicalPath(pcszRemotePath));
   ASSERT(IsValidCNRName(pcszCNRName));
   ASSERT(IS_VALID_WRITE_PTR(ppcnrl, PCNRLINK));
   ASSERT(IS_VALID_WRITE_PTR(pucbCNRLinkLen, UINT));

   /* Determine net provider. */

   bResult = GetNetType(pcszCNRName, &dwNetType);

   if (bResult)
   {
      DWORD dwFlags = ICNRL_FL_VALID_NET_TYPE;

      /* Determine last redirected drive, if any. */

      if (IsDrivePath(pcszRemotePath))
      {
         MyLStrCpyN(rgchDrive, pcszRemotePath, ARRAYSIZE(rgchDrive));
         SET_FLAG(dwFlags, ICNRL_FL_VALID_DEVICE);
      }
      else
         rgchDrive[0] = TEXT('\0');

      bResult = UnifyICNRLinkInfo(pcszCNRName, dwFlags, rgchDrive, dwNetType,
                                  (PICNRLINK *)ppcnrl, pucbCNRLinkLen);
   }

   ASSERT(! bResult ||
          (IS_VALID_STRUCT_PTR((PCICNRLINK)*ppcnrl, CICNRLINK) &&
           EVAL(*pucbCNRLinkLen == GetCNRLinkLen(*ppcnrl))));

   return(bResult);
}


/*
** DestroyCNRLink()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyCNRLink(PCNRLINK pcnrl)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcnrl, CCNRLINK));

   FreeMemory(pcnrl);

   return;
}


/*
** CompareCNRLinks()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** CNR link data is compared in the following order:
**
**    1) net name
**
** N.b., net types are ignored when comparing CNRLinks.
*/
PUBLIC_CODE COMPARISONRESULT CompareCNRLinks(PCCNRLINK pccnrlFirst,
                                             PCCNRLINK pccnrlSecond)
{
#ifdef UNICODE
   WCHAR szWideNetNameFirst[MAX_PATH];
   LPWSTR pszWideNetNameFirst;
   WCHAR szWideNetNameSecond[MAX_PATH];
   LPWSTR pszWideNetNameSecond;
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pccnrlFirst, CCNRLINK));
   ASSERT(IS_VALID_STRUCT_PTR(pccnrlSecond, CCNRLINK));

#ifdef UNICODE
   if (IS_ICNRL_ANSI(pccnrlFirst))
   {
      pszWideNetNameFirst = szWideNetNameFirst;
      MultiByteToWideChar(CP_ACP, 0,
                          ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrlFirst), -1,
                          szWideNetNameFirst, MAX_PATH);

   }
   else
   {
      pszWideNetNameFirst = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrlFirst);
   }

   if (IS_ICNRL_ANSI(pccnrlSecond))
   {
      pszWideNetNameSecond = szWideNetNameSecond;
      MultiByteToWideChar(CP_ACP, 0,
                          ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrlSecond), -1,
                          szWideNetNameSecond, MAX_PATH);

   }
   else
   {
      pszWideNetNameSecond = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrlSecond);
   }

   return(CompareNetNames(pszWideNetNameFirst,pszWideNetNameSecond));
#else
   return(CompareNetNames(ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrlFirst),
                          ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrlSecond)));
#endif
}


/*
** GetLocalPathFromCNRLink()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetLocalPathFromCNRLink(PCCNRLINK pccnrl,
                                         LPTSTR pszLocalPathBuf,
                                         PDWORD pdwOutFlags)
{
   BOOL bResult;
   PCSERVERVTABLE pcsvt;

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszLocalPathBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));

   *pdwOutFlags = 0;

   bResult = GetServerVTable(&pcsvt);

   if (bResult)
   {
      DWORD dwNetType;
      BOOL bIsLocal;

      /*
       * Get local path for share.  N.b., the share name must be in upper case
       * here for MSSHRUI.DLL.
       */

      dwNetType = (IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags,
                               ICNRL_FL_VALID_NET_TYPE) ?
                   ((PCICNRLINK)pccnrl)->dwNetType :
                   0);

#ifdef UNICODE
      {
         WCHAR szWideNetName[MAX_PATH];
         LPWSTR pszWideNetName = szWideNetName;

         if (IS_ICNRL_ANSI(pccnrl))
         {
            MultiByteToWideChar(CP_ACP, 0,
                                ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl), -1,
                                szWideNetName, MAX_PATH);

         }
         else
         {
            pszWideNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
         }
         bResult = (pcsvt->GetLocalPathFromNetResource)(
                        pszWideNetName, dwNetType,
                        pszLocalPathBuf, MAX_PATH_LEN, &bIsLocal);
      }
#else
      bResult = (pcsvt->GetLocalPathFromNetResource)(
                     ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl), dwNetType,
                     pszLocalPathBuf, MAX_PATH_LEN, &bIsLocal);
#endif

      if (bIsLocal)
         SET_FLAG(*pdwOutFlags, CNR_FL_LOCAL);
   }

   ASSERT(FLAGS_ARE_VALID(*pdwOutFlags, ALL_CNR_FLAGS) &&
          (! bResult ||
           (EVAL(IS_FLAG_SET(*pdwOutFlags, CNR_FL_LOCAL)) &&
            EVAL(IsLocalDrivePath(pszLocalPathBuf)))));

   return(bResult);
}


/*
** GetRemotePathFromCNRLink()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void GetRemotePathFromCNRLink(PCCNRLINK pccnrl,
                                          LPTSTR pszRemotePathBuf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRemotePathBuf, STR, MAX_PATH_LEN));

   /* It's ok that this is broken for non-UNC CNR names. */

   /* (- 1) for trailing slash. */

#ifdef UNICODE
   ASSERT(IS_ICNRL_ANSI(pccnrl) ? (lstrlenA(ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl)) < MAX_PATH_LEN - 1) :
                                  (lstrlenW(ICNRL_Remote_Name_PtrW((PCICNRLINK)pccnrl)) < MAX_PATH_LEN - 1));
#else
   ASSERT(lstrlenA(ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl)) < MAX_PATH_LEN - 1);
#endif

#ifdef UNICODE
   {
      WCHAR szWideNetName[MAX_PATH];
      LPWSTR pszWideNetName;

      if (IS_ICNRL_ANSI(pccnrl))
      {
         pszWideNetName = szWideNetName;
         MultiByteToWideChar(CP_ACP, 0,
                             ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl), -1,
                             szWideNetName, MAX_PATH);

      }
      else
      {
         pszWideNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
      }
      lstrcpy(pszRemotePathBuf, pszWideNetName);
   }
#else
   lstrcpy(pszRemotePathBuf, ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl));
#endif
   CatPath(pszRemotePathBuf, TEXT("\\"));

   return;
}


/*
** ConnectToCNR()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ConnectToCNR(PCCNRLINK pccnrl, DWORD dwInFlags,
                              HWND hwndOwner, LPTSTR pszRootPathBuf,
                              PDWORD pdwOutFlags)
{
   BOOL bResult = FALSE;
   BOOL bValidDevice;
   BOOL bRedirect;
   BOOL bTryLastDevice = FALSE;
   DWORD dwcbRootPathBufLen;
   LPTSTR pszNetName;
   LPTSTR pszDevice;
#ifdef UNICODE
   WCHAR szWideNetName[MAX_PATH];
   WCHAR szWideDevice[MAX_PATH];
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));
   ASSERT(FLAGS_ARE_VALID(dwInFlags, ALL_CONNECT_IN_FLAGS));
   ASSERT(IS_FLAG_CLEAR(dwInFlags, CONNECT_INTERACTIVE) ||
          IS_VALID_HANDLE(hwndOwner, WND));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszRootPathBuf, STR, MAX_PATH_LEN));
   ASSERT(IS_VALID_WRITE_PTR(pdwOutFlags, DWORD));

   *pdwOutFlags = 0;

#ifdef UNICODE
   if (IS_ICNRL_ANSI(pccnrl))
   {
      pszNetName = szWideNetName;
      MultiByteToWideChar(CP_ACP, 0,
                          ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl), -1,
                          szWideNetName, MAX_PATH);

   }
   else
   {
      pszNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
   }
#else
   pszNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
#endif

   /* Do we have an old redirected device to try? */

   bValidDevice = IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags,
                              ICNRL_FL_VALID_DEVICE);

#ifdef UNICODE
   if ( bValidDevice )
   {
      if (IS_ICNRL_ANSI(pccnrl))
      {
         pszDevice = szWideDevice;
         MultiByteToWideChar(CP_ACP, 0,
                             ICNRL_Device_PtrA((PCICNRLINK)pccnrl), -1,
                             szWideDevice, MAX_PATH);

      }
      else
      {
         pszDevice = ICNRL_Device_Ptr((PCICNRLINK)pccnrl);
      }
   }
#else
   pszDevice = ICNRL_Device_Ptr((PCICNRLINK)pccnrl);
#endif

   bRedirect = (bValidDevice || IS_FLAG_SET(dwInFlags, CONNECT_REDIRECT));

   if (bRedirect)
   {
      if (bValidDevice)
      {
         DWORD dwNetResult;
         /* "X:" + null terminator */
         TCHAR rgchDrive[2 + 1];

         /* Yes.  Is it already connected to the desired CNR? */

         TRACE_OUT((TEXT("ConnectToCNR(): Calling WNetGetConnection() to check %s for CNR \"%s\"."),
                    pszDevice, pszNetName));

         dwcbRootPathBufLen = MAX_PATH_LEN;

         /* WNetGetConnection requires the device name to have no trailing
         ** backslash.
         */
         MyLStrCpyN(rgchDrive, pszDevice, ARRAYSIZE(rgchDrive));
         dwNetResult = WNetGetConnection(rgchDrive, pszRootPathBuf, &dwcbRootPathBufLen);

         if (dwNetResult == WN_SUCCESS)
         {
            if (CompareNetNames(pszNetName, pszRootPathBuf)
                == CR_EQUAL)
            {
               TRACE_OUT((TEXT("ConnectToCNR(): Found matching CNR \"%s\" on %s."),
                          pszRootPathBuf,
                          pszDevice));

               ASSERT(lstrlenA(ICNRL_Device_PtrA((PCICNRLINK)pccnrl)) < MAX_PATH_LEN);
               lstrcpy(pszRootPathBuf, pszDevice);

               bResult = TRUE;
            }
            else
               TRACE_OUT((TEXT("ConnectToCNR(): Found unmatched CNR \"%s\" on %s."),
                          pszRootPathBuf,
                          pszDevice));
         }
         else
         {
            TRACE_OUT((TEXT("ConnectToCNR(): WNetGetConnection() failed on %s."),
                       pszDevice));

            /*
             * Only attempt a connection to the last redirected device if that
             * device is not already in use.
             */

            bTryLastDevice = (GetDriveType(pszDevice)
                              == DRIVE_NO_ROOT_DIR);
         }
      }

      if (! bResult)
         /* See if the desired CNR is connected to any local device. */
         bResult = SearchForRedirectedConnection((PCICNRLINK)pccnrl,
                                                 pszRootPathBuf);
         /*
          * Assume that no reference count is maintained for redirected device
          * connections, so we do not have to add a found redirected device
          * connection again.
          */
   }

   if (! bResult)
   {
      NETRESOURCE nr;
      TCHAR rgchNPName[MAX_PATH_LEN];

      /* RAIDRAID: (15691) We only support disk resource connections here. */

      ZeroMemory(&nr, SIZEOF(nr));
      nr.lpRemoteName = pszNetName;
      nr.dwType = RESOURCETYPE_DISK;
      if (GetNetProviderName((PCICNRLINK)pccnrl, rgchNPName))
         nr.lpProvider = rgchNPName;

      /* Shall we try the old device? */

      if (bTryLastDevice)
      {
         /* Yes. */

         ASSERT(bValidDevice);

         nr.lpLocalName = pszDevice;

         WARNING_OUT((TEXT("ConnectToCNR(): Calling WNetUseConnection() to attempt to connect %s to CNR \"%s\"."),
                      nr.lpLocalName,
                      nr.lpRemoteName));
      }
      else
      {
         /* No.  Shall we attempt to force a redirected connection? */

         if (bValidDevice)
         {
            /*
             * Yes.  N.b., the caller may already have set CONNECT_REDIRECT in
             * dwInFlags here.
             */

            SET_FLAG(dwInFlags, CONNECT_REDIRECT);

            WARNING_OUT((TEXT("ConnectToCNR(): Calling WNetUseConnection() to establish auto-picked redirected connection to CNR \"%s\"."),
                         nr.lpRemoteName));
         }
         else
            /* No. */
            WARNING_OUT((TEXT("ConnectToCNR(): Calling WNetUseConnection() to establish connection to CNR \"%s\"."),
                         TEXT("<nr.lpRemoteName>"))); // nr.lpRemoteName));

         ASSERT(! nr.lpLocalName);
      }

      dwcbRootPathBufLen = MAX_PATH_LEN;

      bResult = (WNetUseConnection(hwndOwner, &nr, NULL, NULL, dwInFlags,
                                   pszRootPathBuf, &dwcbRootPathBufLen,
                                   pdwOutFlags)
                 == NO_ERROR);
   }

   if (bResult)
      CatPath(pszRootPathBuf, TEXT("\\"));

   ASSERT(! bResult ||
          (IS_VALID_STRING_PTR(pszRootPathBuf, STR) &&
           FLAGS_ARE_VALID(*pdwOutFlags, ALL_CONNECT_OUT_FLAGS)));

   return(bResult);
}


/*
** DisconnectFromCNR()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL DisconnectFromCNR(PCCNRLINK pccnrl)
{
   DWORD dwNetResult;
   LPTSTR pszNetName;
#ifdef UNICODE
   WCHAR szWideNetName[MAX_PATH];
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

#ifdef UNICODE
   if (IS_ICNRL_ANSI(pccnrl))
   {
      pszNetName = szWideNetName;
      MultiByteToWideChar(CP_ACP, 0,
                          ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl), -1,
                          szWideNetName, MAX_PATH);

   }
   else
   {
      pszNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
   }
#else
   pszNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
#endif

   dwNetResult = WNetCancelConnection2(pszNetName,
                                       CONNECT_REFCOUNT, FALSE);

   if (dwNetResult == NO_ERROR)
      WARNING_OUT((TEXT("DisconnectFromCNR(): Reduced connection reference count on CNR \"%s\"."),
                   pszNetName));
   else
      WARNING_OUT((TEXT("DisconnectFromCNR(): Failed to reduce connection reference count on CNR \"%s\".  WNetCancelConnection2() returned %lu."),
                   pszNetName));

   return(dwNetResult == NO_ERROR);
}


/*
** IsCNRAvailable()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsCNRAvailable(PCCNRLINK pccnrl)
{
   TCHAR rgchCNRRoot[MAX_PATH_LEN];
   LPTSTR pszNetName;
#ifdef UNICODE
   WCHAR szWideNetName[MAX_PATH];
#endif

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

#ifdef UNICODE
   if (IS_ICNRL_ANSI(pccnrl))
   {
      pszNetName = szWideNetName;
      MultiByteToWideChar(CP_ACP, 0,
                          ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl), -1,
                          szWideNetName, MAX_PATH);

   }
   else
   {
      pszNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
   }
#else
   pszNetName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
#endif

   ASSERT(lstrlen(pszNetName) < ARRAYSIZE(rgchCNRRoot) - 1);
   lstrcpy(rgchCNRRoot, pszNetName);
   CatPath(rgchCNRRoot, TEXT("\\"));

   return(PathExists(rgchCNRRoot));
}


/*
** GetCNRLinkLen()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE UINT GetCNRLinkLen(PCCNRLINK pccnrl)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   return(((PCICNRLINK)pccnrl)->ucbSize);
}


/*
** GetCNRNetType()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetCNRNetType(PCCNRLINK pccnrl, PCDWORD *ppcdwNetType)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   bResult = IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags,
                         ICNRL_FL_VALID_NET_TYPE);

   if (bResult)
      *ppcdwNetType = &(((PCICNRLINK)pccnrl)->dwNetType);

   ASSERT(! bResult ||
          IsValidNetType(**ppcdwNetType));

   return(bResult);
}


/*
** GetCNRName()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetCNRName(PCCNRLINK pccnrl, LPCSTR *ppcszCNRName)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   *ppcszCNRName = ICNRL_Remote_Name_PtrA((PCICNRLINK)pccnrl);

   ASSERT(IS_VALID_STRING_PTRA(*ppcszCNRName, CSTR));

   return(TRUE);
}

#ifdef UNICODE
/*
** GetCNRNameW()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetCNRNameW(PCCNRLINK pccnrl, LPCWSTR *ppcszCNRName)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   if (IS_ICNRL_ANSI(pccnrl))
      *ppcszCNRName = NULL;
   else
   {
      *ppcszCNRName = ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl);
      ASSERT(IS_VALID_STRING_PTR(*ppcszCNRName, CSTR));
   }

   return(TRUE);
}
#endif

/*
** GetLastRedirectedDevice()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetLastRedirectedDevice(PCCNRLINK pccnrl, LPCSTR *ppcszDevice)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   bResult = IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags, ICNRL_FL_VALID_DEVICE);

   if (bResult)
      *ppcszDevice = ICNRL_Device_PtrA((PCICNRLINK)pccnrl);

   ASSERT(! bResult ||
          IS_VALID_STRING_PTRA(*ppcszDevice, CSTR));

   return(bResult);
}

#ifdef UNICODE
/*
** GetLastRedirectedDeviceW()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL GetLastRedirectedDeviceW(PCCNRLINK pccnrl, LPCWSTR *ppcszDevice)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   bResult = IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags, ICNRL_FL_VALID_DEVICE);

   if (bResult)
      if (IS_ICNRL_ANSI(pccnrl))
         *ppcszDevice = NULL;
      else
      {
         *ppcszDevice = ICNRL_Device_Ptr((PCICNRLINK)pccnrl);
         ASSERT(! bResult ||
               IS_VALID_STRING_PTR(*ppcszDevice, CSTR));
      }

   return(bResult);
}
#endif

#if defined(DEBUG) || defined (VSTF)

/*
** IsValidPCCNRLINK()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidPCCNRLINK(PCCNRLINK pccnrl)
{
   return(IS_VALID_STRUCT_PTR((PCICNRLINK)pccnrl, CICNRLINK));
}

#endif


#ifdef DEBUG

/*
** DumpCNRLink()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE void DumpCNRLink(PCCNRLINK pccnrl)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccnrl, CCNRLINK));

   PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] ucbSize %#x"),
                    INDENT_STRING,
                    INDENT_STRING,
                    ((PCICNRLINK)pccnrl)->ucbSize));
   PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] dwFLags = %#08lx"),
                    INDENT_STRING,
                    INDENT_STRING,
                    ((PCICNRLINK)pccnrl)->dwFlags));
   PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] CNR name \"%s\""),
                    INDENT_STRING,
                    INDENT_STRING,
                    ICNRL_Remote_Name_Ptr((PCICNRLINK)pccnrl)));
   if (IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags, ICNRL_FL_VALID_NET_TYPE))
      PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] net type %#08lx"),
                       INDENT_STRING,
                       INDENT_STRING,
                       ((PCICNRLINK)pccnrl)->dwNetType));
   else
      PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] net type unknown"),
                       INDENT_STRING,
                       INDENT_STRING));
   if (IS_FLAG_SET(((PCICNRLINK)pccnrl)->dwFlags, ICNRL_FL_VALID_DEVICE))
      PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] last redirected local device \"%s\""),
                       INDENT_STRING,
                       INDENT_STRING,
                       ICNRL_Device_Ptr((PCICNRLINK)pccnrl)));
   else
      PLAIN_TRACE_OUT((TEXT("%s%s[CNR link] no last redirected local device"),
                       INDENT_STRING,
                       INDENT_STRING));

   return;
}

#endif
