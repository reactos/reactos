//
// Mime stuff used by shell32/shdocvw/shdoc401
//
#include "priv.h"


#define TF_MIME 0

/*----------------------------------------------------------
Purpose: Generates the HKEY_CLASSES_ROOT subkey for a MIME
         type

Returns: 
Cond:    --
*/
STDAPI_(BOOL) GetMIMETypeSubKeyA(LPCSTR pcszMIMEType, LPSTR pszSubKeyBuf, UINT cchBuf)
{
    BOOL bResult;

    bResult = ((UINT)lstrlenA(TEXT("MIME\\Database\\Content Type\\%s")) +
               (UINT)lstrlenA(pcszMIMEType) < cchBuf);

    if (bResult)
        EVAL((UINT)wsprintfA(pszSubKeyBuf, TEXT("MIME\\Database\\Content Type\\%s"),
                             pcszMIMEType) < cchBuf);
    else
    {
        if (cchBuf > 0)
           *pszSubKeyBuf = '\0';

        TraceMsg(TF_WARNING, "GetMIMETypeSubKey(): Given sub key buffer of length %u is too short to hold sub key for MIME type %hs.",
                     cchBuf, pcszMIMEType);
    }

    ASSERT(! cchBuf ||
           (IS_VALID_STRING_PTRA(pszSubKeyBuf, -1) &&
            (UINT)lstrlenA(pszSubKeyBuf) < cchBuf));
    ASSERT(bResult ||
           ! cchBuf ||
           ! *pszSubKeyBuf);

    return(bResult);
}


STDAPI_(BOOL) GetMIMETypeSubKeyW(LPCWSTR pszMIMEType, LPWSTR pszBuf, UINT cchBuf)
{
    BOOL bRet;
    char szMIMEType[MAX_PATH];
    char sz[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pszMIMEType, -1));

    WideCharToMultiByte(CP_ACP, 0, pszMIMEType, -1, szMIMEType, SIZECHARS(szMIMEType), NULL, NULL);
    bRet = GetMIMETypeSubKeyA(szMIMEType, sz, SIZECHARS(sz));

    if (bRet)
    {
        ASSERT(cchBuf <= SIZECHARS(sz));
        MultiByteToWideChar(CP_ACP, 0, sz, -1, pszBuf, cchBuf);
    }
    return bRet;
}    


/*
** RegisterExtensionForMIMEType()
**
** Under HKEY_CLASSES_ROOT\MIME\Database\Content Type\mime/type, add
** Content Type = mime/type and Extension = .ext.
**
*/
STDAPI_(BOOL) RegisterExtensionForMIMETypeA(LPCSTR pcszExtension, LPCSTR pcszMIMEContentType)
{
    BOOL bResult;
    CHAR szMIMEContentTypeSubKey[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRA(pcszExtension, -1));
    ASSERT(IS_VALID_STRING_PTRA(pcszMIMEContentType, -1));

    ASSERT(IsValidExtensionA(pcszExtension));

    bResult = GetMIMETypeSubKeyA(pcszMIMEContentType, szMIMEContentTypeSubKey,
                                SIZECHARS(szMIMEContentTypeSubKey));

    if (bResult)
    {
        /* (+ 1) for null terminator. */
        bResult = (NO_ERROR == SHSetValueA(HKEY_CLASSES_ROOT, szMIMEContentTypeSubKey,
                                          "Extension", REG_SZ, pcszExtension,
                                          CbFromCchA(lstrlenA(pcszExtension) + 1)));
    }

    return(bResult);
}


STDAPI_(BOOL) RegisterExtensionForMIMETypeW(LPCWSTR pcszExtension, LPCWSTR pcszMIMEContentType)
{
    BOOL bResult;
    WCHAR szMIMEContentTypeSubKey[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pcszExtension, -1));
    ASSERT(IS_VALID_STRING_PTRW(pcszMIMEContentType, -1));

    ASSERT(IsValidExtensionW(pcszExtension));

    bResult = GetMIMETypeSubKeyW(pcszMIMEContentType, szMIMEContentTypeSubKey,
                                SIZECHARS(szMIMEContentTypeSubKey));

    if (bResult)
    {
        /* (+ 1) for null terminator. */
        bResult = (NO_ERROR == SHSetValueW(HKEY_CLASSES_ROOT, szMIMEContentTypeSubKey,
                                          TEXTW("Extension"), REG_SZ, pcszExtension,
                                          (lstrlenW(pcszExtension) + 1)*sizeof(WCHAR)));
    }

    return(bResult);
}


/*
** UnregisterExtensionForMIMEType()
**
** Deletes Extension under
** HKEY_CLASSES_ROOT\MIME\Database\Content Type\mime/type.  If no other values
** or sub keys are left, deletes
** HKEY_CLASSES_ROOT\MIME\Database\Content Type\mime/type.
**
** Side Effects:  May also delete MIME key.
*/
STDAPI_(BOOL) UnregisterExtensionForMIMETypeA(LPCSTR pcszMIMEContentType)
{
    BOOL bResult;
    CHAR szMIMEContentTypeSubKey[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRA(pcszMIMEContentType, -1));

    bResult = (GetMIMETypeSubKeyA(pcszMIMEContentType, szMIMEContentTypeSubKey,
                                 SIZECHARS(szMIMEContentTypeSubKey)) &&
               NO_ERROR == SHDeleteValueA(HKEY_CLASSES_ROOT, szMIMEContentTypeSubKey,
                                         "Extension") &&
               NO_ERROR == SHDeleteEmptyKeyA(HKEY_CLASSES_ROOT, szMIMEContentTypeSubKey));

    return(bResult);
}


STDAPI_(BOOL) UnregisterExtensionForMIMETypeW(LPCWSTR pcszMIMEContentType)
{
    BOOL bResult;
    WCHAR szMIMEContentTypeSubKey[MAX_PATH];

    ASSERT(IS_VALID_STRING_PTRW(pcszMIMEContentType, -1));

    bResult = (GetMIMETypeSubKeyW(pcszMIMEContentType, szMIMEContentTypeSubKey,
                                 SIZECHARS(szMIMEContentTypeSubKey)) &&
               NO_ERROR == SHDeleteValueW(HKEY_CLASSES_ROOT, szMIMEContentTypeSubKey,
                                         TEXTW("Extension")) &&
               NO_ERROR == SHDeleteEmptyKeyW(HKEY_CLASSES_ROOT, szMIMEContentTypeSubKey));

    return(bResult);
}


/*
** UnregisterMIMETypeForExtension()
**
** Deletes Content Type under HKEY_CLASSES_ROOT\.ext.
**
** Side Effects:  none
*/
STDAPI_(BOOL) UnregisterMIMETypeForExtensionA(LPCSTR pcszExtension)
{
    ASSERT(IS_VALID_STRING_PTRA(pcszExtension, -1));
    ASSERT(IsValidExtensionA(pcszExtension));

    return NO_ERROR == SHDeleteValueA(HKEY_CLASSES_ROOT, pcszExtension, "Content Type");
}


STDAPI_(BOOL) UnregisterMIMETypeForExtensionW(LPCWSTR pcszExtension)
{
    ASSERT(IS_VALID_STRING_PTRW(pcszExtension, -1));
    ASSERT(IsValidExtensionW(pcszExtension));

    return NO_ERROR == SHDeleteValueW(HKEY_CLASSES_ROOT, pcszExtension, TEXTW("Content Type"));
}


/*
** RegisterMIMETypeForExtension()
**
** Under HKEY_CLASSES_ROOT\.ext, add Content Type = mime/type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
STDAPI_(BOOL) RegisterMIMETypeForExtensionA(LPCSTR pcszExtension, LPCSTR pcszMIMEContentType)
{
    ASSERT(IS_VALID_STRING_PTRA(pcszExtension, -1));
    ASSERT(IS_VALID_STRING_PTRA(pcszMIMEContentType, -1));

    ASSERT(IsValidExtensionA(pcszExtension));

    /* (+ 1) for null terminator. */
    return NO_ERROR == SHSetValueA(HKEY_CLASSES_ROOT, pcszExtension, "Content Type", 
                                  REG_SZ, pcszMIMEContentType,
                                  CbFromCchA(lstrlenA(pcszMIMEContentType) + 1));
}


STDAPI_(BOOL) RegisterMIMETypeForExtensionW(LPCWSTR pcszExtension, LPCWSTR pcszMIMEContentType)
{
    ASSERT(IS_VALID_STRING_PTRW(pcszExtension, -1));
    ASSERT(IS_VALID_STRING_PTRW(pcszMIMEContentType, -1));
    ASSERT(IsValidExtensionW(pcszExtension));

    /* (+ 1) for null terminator. */
    return NO_ERROR == SHSetValueW(HKEY_CLASSES_ROOT, pcszExtension, TEXTW("Content Type"), 
                                  REG_SZ, pcszMIMEContentType,
                                  (lstrlenW(pcszMIMEContentType) + 1) * sizeof(WCHAR));
}

/*
** GetMIMEValue()
**
** Retrieves the data for a value of a MIME type.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
STDAPI_(BOOL) GetMIMEValueA(LPCSTR pcszMIMEType, LPCSTR pcszValue,
                              PDWORD pdwValueType, PBYTE pbyteValueBuf,
                              PDWORD pdwcbValueBufLen)
{
   BOOL bResult;
   CHAR szMIMETypeSubKey[MAX_PATH];

   ASSERT(IS_VALID_STRING_PTRA(pcszMIMEType, -1));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTRA(pcszValue, -1));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   bResult = (GetMIMETypeSubKeyA(pcszMIMEType, szMIMETypeSubKey,SIZECHARS(szMIMETypeSubKey)) &&
              NO_ERROR == SHGetValueA(HKEY_CLASSES_ROOT, szMIMETypeSubKey,
                                      pcszValue, pdwValueType, pbyteValueBuf,
                                      pdwcbValueBufLen));

   return(bResult);
}

STDAPI_(BOOL) GetMIMEValueW(LPCWSTR pcszMIMEType, LPCWSTR pcszValue,
                              PDWORD pdwValueType, PBYTE pbyteValueBuf,
                              PDWORD pdwcbValueBufLen)
{
   BOOL bResult;
   WCHAR szMIMETypeSubKey[MAX_PATH];

   ASSERT(IS_VALID_STRING_PTRW(pcszMIMEType, -1));
   ASSERT(! pcszValue ||
          IS_VALID_STRING_PTRW(pcszValue, -1));
   ASSERT(IS_VALID_WRITE_PTR(pdwValueType, DWORD));
   ASSERT(IS_VALID_WRITE_BUFFER(pbyteValueBuf, BYTE, *pdwcbValueBufLen));

   bResult = (GetMIMETypeSubKeyW(pcszMIMEType, szMIMETypeSubKey,SIZECHARS(szMIMETypeSubKey)) &&
              NO_ERROR == SHGetValueW(HKEY_CLASSES_ROOT, szMIMETypeSubKey,
                                      pcszValue, pdwValueType, pbyteValueBuf,
                                      pdwcbValueBufLen));

   return(bResult);
}

/*
** GetMIMETypeStringValue()
**
** Retrieves the string for a registered MIME type's value.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
STDAPI_(BOOL) GetMIMETypeStringValueA(LPCSTR pcszMIMEType, LPCSTR pcszValue,
                                         LPSTR pszBuf, UINT ucBufLen)
{
   BOOL bResult;
   DWORD dwValueType;
   DWORD dwcbLen = CbFromCchA(ucBufLen);

   /* GetMIMEValue() will verify parameters. */

   bResult = (GetMIMEValueA(pcszMIMEType, pcszValue, &dwValueType, (PBYTE)pszBuf, &dwcbLen) &&
              dwValueType == REG_SZ);

   if (! bResult)
   {
      if (ucBufLen > 0)
         *pszBuf = '\0';
   }

   ASSERT(! ucBufLen || IS_VALID_STRING_PTRA(pszBuf, -1));

   return(bResult);
}

STDAPI_(BOOL) GetMIMETypeStringValueW(LPCWSTR pcszMIMEType, LPCWSTR pcszValue,
                                         LPWSTR pszBuf, UINT ucBufLen)
{
   BOOL bResult;
   DWORD dwValueType;
   DWORD dwcbLen = CbFromCchW(ucBufLen);

   /* GetMIMEValue() will verify parameters. */

   bResult = (GetMIMEValueW(pcszMIMEType, pcszValue, &dwValueType, (PBYTE)pszBuf, &dwcbLen) &&
              dwValueType == REG_SZ);

   if (! bResult)
   {
      if (ucBufLen > 0)
         *pszBuf = '\0';
   }

   ASSERT(! ucBufLen || IS_VALID_STRING_PTRW(pszBuf, -1));

   return(bResult);
}


/*
** MIME_GetExtension()
**
** Determines the file name extension to be used when writing a file of a MIME
** type to the file system.
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
STDAPI_(BOOL) MIME_GetExtensionA(LPCSTR pcszMIMEType, LPSTR pszExtensionBuf, UINT ucExtensionBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRING_PTRA(pcszMIMEType, -1));
   ASSERT(IS_VALID_WRITE_BUFFER(pszExtensionBuf, CHAR, ucExtensionBufLen));

   if (EVAL(ucExtensionBufLen > 2))
   {
      /* Leave room for possible leading period. */

      if (GetMIMETypeStringValueA(pcszMIMEType, "Extension", pszExtensionBuf + 1, ucExtensionBufLen - 1))
      {
         if (pszExtensionBuf[1])
         {
            /* Prepend period if necessary. */

            if (pszExtensionBuf[1] == TEXT('.'))
               /* (+ 1) for null terminator. */
               MoveMemory(pszExtensionBuf, pszExtensionBuf + 1,
                          CbFromCchA(lstrlenA(pszExtensionBuf + 1) + 1));
            else
               pszExtensionBuf[0] = TEXT('.');

            bResult = TRUE;
         }
      }
   }

   if (! bResult)
   {
      if (ucExtensionBufLen > 0)
         *pszExtensionBuf = '\0';
   }

   if (bResult)
      TraceMsgA(TF_MIME, "MIME_GetExtension(): Extension %s registered as default extension for MIME type %s.",
                 pszExtensionBuf, pcszMIMEType);

   ASSERT((bResult &&
           IsValidExtensionA(pszExtensionBuf)) ||
          (! bResult &&
           (! ucExtensionBufLen ||
            ! *pszExtensionBuf)));
   ASSERT(! ucExtensionBufLen ||
          (UINT)lstrlenA(pszExtensionBuf) < ucExtensionBufLen);

   return(bResult);
}


STDAPI_(BOOL) MIME_GetExtensionW(LPCWSTR pcszMIMEType, LPWSTR pszExtensionBuf, UINT ucExtensionBufLen)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRING_PTRW(pcszMIMEType, -1));
   ASSERT(IS_VALID_WRITE_BUFFER(pszExtensionBuf, CHAR, ucExtensionBufLen));

   if (EVAL(ucExtensionBufLen > 2))
   {
      /* Leave room for possible leading period. */

      if (GetMIMETypeStringValueW(pcszMIMEType, TEXTW("Extension"), pszExtensionBuf + 1, ucExtensionBufLen - 1))
      {
         if (pszExtensionBuf[1])
         {
            /* Prepend period if necessary. */

            if (pszExtensionBuf[1] == TEXT('.'))
               /* (+ 1) for null terminator. */
               MoveMemory(pszExtensionBuf, pszExtensionBuf + 1,
                          CbFromCchW(lstrlenW(pszExtensionBuf + 1) + 1));
            else
               pszExtensionBuf[0] = TEXT('.');

            bResult = TRUE;
         }
      }
   }

   if (! bResult)
   {
      if (ucExtensionBufLen > 0)
         *pszExtensionBuf = '\0';
   }

   if (bResult)
      TraceMsgW(TF_MIME, "MIME_GetExtension(): Extension %s registered as default extension for MIME type %s.",
                 pszExtensionBuf, pcszMIMEType);

   ASSERT((bResult &&
           IsValidExtensionW(pszExtensionBuf)) ||
          (! bResult &&
           (! ucExtensionBufLen ||
            ! *pszExtensionBuf)));
   ASSERT(! ucExtensionBufLen ||
          (UINT)lstrlenW(pszExtensionBuf) < ucExtensionBufLen);

   return(bResult);
}


