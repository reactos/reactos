/*
 * string.c - String table ADT module.
 */

/*

   The string table ADT implemented in this module is set up as a hash table
with HASH_TABLE_SIZE buckets.  A hash function is calculated for each string to
determine its bucket.  Multiple strings in a single bucket are stored in a
linked list.  The string hash table allows us to keep only one copy of a string
that is used multiple times.  Strings are allocated in the heap by
AllocateMemory().

   Every string has a list node structure associated with it.  A string is
accessed through its associated list node.  Each hash bucket is a list of
string nodes.  A handle to a string table is a pointer to the base of the
string table's array of hash buckets.  String tables are allocated in the heap
by AllocateMemory().  Each element in an array of hash buckets is a handle to a
list of strings in the hash bucket.  A handle to a string is a handle to a node
in the string's hash bucket's list.

   Hash table ADTs are predicated on the idea that hash buckets will typically
be shallow, so the search of a hash bucket will not take horrendously long.
The data objects in hash buckets should be stored in sorted order to reduce
search time.  If hash buckets get too deep, increase the hash table size.
Ideally, the hash table should be implemented as a container class that hashes
arbitrary data objects given an initial hash table size, the size of the
objects to be hashed, a hash function, and a data object comparison function.

   Currently the hash table ADT is restricted to strings, the strings in each
hash bucket are stored in sorted order, and hash buckets are binary searched.

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Types
 ********/

/* string table */

typedef struct _stringtable
{
   /* number of hash buckets in string table */

   HASHBUCKETCOUNT hbc;

   /* pointer to array of hash buckets (HLISTs) */

   PHLIST phlistHashBuckets;
}
STRINGTABLE;
DECLARE_STANDARD_TYPES(STRINGTABLE);

/* string heap structure */

typedef struct _string
{
   /* lock count of string */

   ULONG ulcLock;

   /* actual string */

   TCHAR string[1];
}
STRING;
DECLARE_STANDARD_TYPES(STRING);

/* string table database structure header */

typedef struct _stringtabledbheader
{
   /*
    * length of longest string in string table, not including null terminator
    */

   DWORD dwcbMaxStringLen;

   /* number of strings in string table */

   LONG lcStrings;
}
STRINGTABLEDBHEADER;
DECLARE_STANDARD_TYPES(STRINGTABLEDBHEADER);

/* database string header */

typedef struct _dbstringheader
{
   /* old handle to this string */

   HSTRING hsOld;
}
DBSTRINGHEADER;
DECLARE_STANDARD_TYPES(DBSTRINGHEADER);


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE COMPARISONRESULT StringSearchCmp(PCVOID, PCVOID);
PRIVATE_CODE COMPARISONRESULT StringSortCmp(PCVOID, PCVOID);
PRIVATE_CODE BOOL UnlockString(PSTRING);
PRIVATE_CODE BOOL FreeStringWalker(PVOID, PVOID);
PRIVATE_CODE void FreeHashBucket(HLIST);
PRIVATE_CODE TWINRESULT WriteHashBucket(HCACHEDFILE, HLIST, PLONG, PDWORD);
PRIVATE_CODE TWINRESULT WriteString(HCACHEDFILE, HNODE, PSTRING, PDWORD);
PRIVATE_CODE TWINRESULT ReadString(HCACHEDFILE, HSTRINGTABLE, HHANDLETRANS, LPTSTR, DWORD);
PRIVATE_CODE TWINRESULT SlowReadString(HCACHEDFILE, LPTSTR, DWORD);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCNEWSTRINGTABLE(PCNEWSTRINGTABLE);
PRIVATE_CODE BOOL IsValidPCSTRING(PCSTRING);
PRIVATE_CODE BOOL IsValidPCSTRINGTABLE(PCSTRINGTABLE);

#endif


/*
** StringSearchCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT StringSearchCmp(PCVOID pcszPath, PCVOID pcstring)
{
   ASSERT(IS_VALID_STRING_PTR(pcszPath, CSTR));
   ASSERT(IS_VALID_STRUCT_PTR(pcstring, CSTRING));

   return(MapIntToComparisonResult(lstrcmp((LPCTSTR)pcszPath,
                                           (LPCTSTR)&(((PCSTRING)pcstring)->string))));
}


/*
** StringSortCmp()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE COMPARISONRESULT StringSortCmp(PCVOID pcstring1, PCVOID pcstring2)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcstring1, CSTRING));
   ASSERT(IS_VALID_STRUCT_PTR(pcstring2, CSTRING));

   return(MapIntToComparisonResult(lstrcmp((LPCTSTR)&(((PCSTRING)pcstring1)->string),
                                           (LPCTSTR)&(((PCSTRING)pcstring2)->string))));
}


/*
** UnlockString()
**
** Decrements a string's lock count.
**
** Arguments:
**
** Returns:       void
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL UnlockString(PSTRING pstring)
{
   ASSERT(IS_VALID_STRUCT_PTR(pstring, CSTRING));

   /* Is the lock count going to underflow? */

   if (EVAL(pstring->ulcLock > 0))
      pstring->ulcLock--;

   return(pstring->ulcLock > 0);
}


/*
** FreeStringWalker()
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

PRIVATE_CODE BOOL FreeStringWalker(PVOID pstring, PVOID pvUnused)
{
   ASSERT(IS_VALID_STRUCT_PTR(pstring, CSTRING));
   ASSERT(! pvUnused);

   FreeMemory(pstring);

   return(TRUE);
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** FreeHashBucket()
**
** Frees the strings in a hash bucket, and the hash bucket's string list.
**
** Arguments:     hlistHashBucket - handle to hash bucket's list of strings
**
** Returns:       void
**
** Side Effects:  none
**
** N.b., this function ignores the lock counts of the strings in the hash
** bucket.  All strings in the hash bucket are freed.
*/
PRIVATE_CODE void FreeHashBucket(HLIST hlistHashBucket)
{
   ASSERT(! hlistHashBucket || IS_VALID_HANDLE(hlistHashBucket, LIST));

   /* Are there any strings in this hash bucket to delete? */

   if (hlistHashBucket)
   {
      /* Yes.  Delete all strings in list. */

      EVAL(WalkList(hlistHashBucket, &FreeStringWalker, NULL));

      /* Delete hash bucket string list. */

      DestroyList(hlistHashBucket);
   }

   return;
}


/*
** MyGetStringLen()
**
** Retrieves the length of a string in a string table.
**
** Arguments:     pcstring - pointer to string whose length is to be
**                            determined
**
** Returns:       Length of string in bytes, not including null terminator.
**
** Side Effects:  none
*/
PRIVATE_CODE int MyGetStringLen(PCSTRING pcstring)
{
   ASSERT(IS_VALID_STRUCT_PTR(pcstring, CSTRING));

   return(lstrlen(pcstring->string) * sizeof(TCHAR));
}


/*
** WriteHashBucket()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WriteHashBucket(HCACHEDFILE hcf,
                                           HLIST hlistHashBucket,
                                           PLONG plcStrings,
                                           PDWORD pdwcbMaxStringLen)
{
   TWINRESULT tr = TR_SUCCESS;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(! hlistHashBucket || IS_VALID_HANDLE(hlistHashBucket, LIST));
   ASSERT(IS_VALID_WRITE_PTR(plcStrings, LONG));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbMaxStringLen, DWORD));

   /* Any strings in this hash bucket? */

   *plcStrings = 0;
   *pdwcbMaxStringLen = 0;

   if (hlistHashBucket)
   {
      BOOL bContinue;
      HNODE hnode;

      /* Yes.  Walk hash bucket, saving each string. */

      for (bContinue = GetFirstNode(hlistHashBucket, &hnode);
           bContinue;
           bContinue = GetNextNode(hnode, &hnode))
      {
         PSTRING pstring;

         pstring = (PSTRING)GetNodeData(hnode);

         ASSERT(IS_VALID_STRUCT_PTR(pstring, CSTRING));

         /*
          * As a sanity check, don't save any string with a lock count of 0.  A
          * 0 lock count implies that the string has not been referenced since
          * it was restored from the database, or something is broken.
          */

         if (pstring->ulcLock > 0)
         {
            DWORD dwcbStringLen;

            tr = WriteString(hcf, hnode, pstring, &dwcbStringLen);

            if (tr == TR_SUCCESS)
            {
               if (dwcbStringLen > *pdwcbMaxStringLen)
                  *pdwcbMaxStringLen = dwcbStringLen;

               ASSERT(*plcStrings < LONG_MAX);
               (*plcStrings)++;
            }
            else
               break;
         }
         else
            ERROR_OUT((TEXT("WriteHashBucket(): String \"%s\" has 0 lock count and will not be saved."),
                       pstring->string));
      }
   }

   return(tr);
}


/*
** WriteString()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT WriteString(HCACHEDFILE hcf, HNODE hnodeOld,
                                    PSTRING pstring, PDWORD pdwcbStringLen)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DBSTRINGHEADER dbsh;

   /* (+ 1) for null terminator. */

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hnodeOld, NODE));
   ASSERT(IS_VALID_STRUCT_PTR(pstring, CSTRING));
   ASSERT(IS_VALID_READ_BUFFER_PTR(pstring, STRING, sizeof(STRING) + MyGetStringLen(pstring) + sizeof(TCHAR) - sizeof(pstring->string)));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbStringLen, DWORD));

   /* Create string header. */

   dbsh.hsOld = (HSTRING)hnodeOld;

   /* Save string header and string. */

   if (WriteToCachedFile(hcf, (PCVOID)&dbsh, sizeof(dbsh), NULL))
   {
      LPSTR pszAnsi;

      /* (+ 1) for null terminator. */

      *pdwcbStringLen = MyGetStringLen(pstring) + SIZEOF(TCHAR);

      // If its unicode, convert the string to ansi before writing it out

      #ifdef UNICODE
      {
          pszAnsi = LocalAlloc(LPTR, *pdwcbStringLen);
          if (NULL == pszAnsi)
          {
            return tr;
          }
          WideCharToMultiByte(CP_ACP, 0, pstring->string, -1, pszAnsi, *pdwcbStringLen, NULL, NULL);

          // We should always have a string at this point that can be converted losslessly

          #if (defined(DEBUG) || defined(DBG)) && defined(UNICODE)
          {
                WCHAR szUnicode[MAX_PATH*2];
                MultiByteToWideChar(CP_ACP, 0, pszAnsi, -1, szUnicode, ARRAYSIZE(szUnicode));
                ASSERT(0 == lstrcmp(szUnicode, pstring->string));
          }
          #endif

          if (WriteToCachedFile(hcf, (PCVOID) pszAnsi, lstrlenA(pszAnsi) + 1, NULL))
            tr = TR_SUCCESS;

          LocalFree(pszAnsi);
     }
     #else
      
          if (WriteToCachedFile(hcf, (PCVOID)&(pstring->string), (UINT)*pdwcbStringLen, NULL))
             tr = TR_SUCCESS;
 
     #endif

   }

   return(tr);
}


/*
** ReadString()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT ReadString(HCACHEDFILE hcf, HSTRINGTABLE hst,
                                      HHANDLETRANS hht, LPTSTR pszStringBuf,
                                      DWORD dwcbStringBufLen)
{
   TWINRESULT tr;
   DBSTRINGHEADER dbsh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
   ASSERT(IS_VALID_HANDLE(hht, HANDLETRANS));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszStringBuf, STR, (UINT)dwcbStringBufLen));

   if (ReadFromCachedFile(hcf, &dbsh, sizeof(dbsh), &dwcbRead) &&
       dwcbRead == sizeof(dbsh))
   {
      tr = SlowReadString(hcf, pszStringBuf, dwcbStringBufLen);

      if (tr == TR_SUCCESS)
      {
         HSTRING hsNew;

         if (AddString(pszStringBuf, hst, GetHashBucketIndex, &hsNew))
         {
            /*
             * We must undo the LockString() performed by AddString() to
             * maintain the correct string lock count.  N.b., the lock count of
             * a string may be > 0 even after unlocking since the client may
             * already have added the string to the given string table.
             */

            UnlockString((PSTRING)GetNodeData((HNODE)hsNew));

            if (! AddHandleToHandleTranslator(hht, (HGENERIC)(dbsh.hsOld), (HGENERIC)hsNew))
            {
               DeleteNode((HNODE)hsNew);

               tr = TR_CORRUPT_BRIEFCASE;
            }
         }
         else
            tr = TR_OUT_OF_MEMORY;
      }
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


/*
** SlowReadString()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE TWINRESULT SlowReadString(HCACHEDFILE hcf, LPTSTR pszStringBuf,
                                          DWORD dwcbStringBufLen)
{
   TWINRESULT tr = TR_CORRUPT_BRIEFCASE;
   LPTSTR pszStringBufEnd;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(pszStringBuf, STR, (UINT)dwcbStringBufLen));

   pszStringBufEnd = pszStringBuf + dwcbStringBufLen;

   // The database strings are always written ANSI, so if we are running unicode,
   // we need to convert as we go

   #ifdef UNICODE
   {
        LPSTR pszAnsiEnd;
        LPSTR pszAnsiStart;
        LPSTR pszAnsi = LocalAlloc(LPTR, dwcbStringBufLen);
        pszAnsiStart  = pszAnsi;
        pszAnsiEnd    = pszAnsi + dwcbStringBufLen;
            
        if (NULL == pszAnsi)
        {
            return tr;
        }

        while (pszAnsi < pszAnsiEnd &&
              ReadFromCachedFile(hcf, pszAnsi, sizeof(*pszAnsi), &dwcbRead) &&
              dwcbRead == sizeof(*pszAnsi))
        {
            if (*pszAnsi)
                pszAnsi++;
            else
            {
                tr = TR_SUCCESS;
                break;
            }
        }

       if (tr == TR_SUCCESS)
       {
            MultiByteToWideChar(CP_ACP, 0, pszAnsiStart, -1, pszStringBuf, dwcbStringBufLen / sizeof(TCHAR));
       }

       LocalFree(pszAnsiStart);
    }
    #else

       while (pszStringBuf < pszStringBufEnd &&
              ReadFromCachedFile(hcf, pszStringBuf, sizeof(*pszStringBuf), &dwcbRead) &&
              dwcbRead == sizeof(*pszStringBuf))
       {
          if (*pszStringBuf)
             pszStringBuf++;
          else
          {
             tr = TR_SUCCESS;
             break;
          }
       }

    #endif

   return(tr);
}


#ifdef VSTF

/*
** IsValidPCNEWSTRINGTABLE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCNEWSTRINGTABLE(PCNEWSTRINGTABLE pcnst)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pcnst, CNEWSTRINGTABLE) &&
       EVAL(pcnst->hbc > 0))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidPCSTRING()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCSTRING(PCSTRING pcs)
{
   BOOL bResult;

   if (IS_VALID_READ_PTR(pcs, CSTRING) &&
       IS_VALID_STRING_PTR(pcs->string, CSTR))
      bResult = TRUE;
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidStringWalker()
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

PRIVATE_CODE BOOL IsValidStringWalker(PVOID pstring, PVOID pvUnused)
{
   ASSERT(! pvUnused);

   return(IS_VALID_STRUCT_PTR(pstring, CSTRING));
}

#pragma warning(default:4100) /* "unreferenced formal parameter" warning */


/*
** IsValidPCSTRINGTABLE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCSTRINGTABLE(PCSTRINGTABLE pcst)
{
   BOOL bResult = FALSE;

   if (IS_VALID_READ_PTR(pcst, CSTRINGTABLE) &&
       EVAL(pcst->hbc > 0) &&
       IS_VALID_READ_BUFFER_PTR(pcst->phlistHashBuckets, HLIST, pcst->hbc * sizeof((pcst->phlistHashBuckets)[0])))
   {
      HASHBUCKETCOUNT hbc;

      for (hbc = 0; hbc < pcst->hbc; hbc++)
      {
         HLIST hlistHashBucket;

         hlistHashBucket = (pcst->phlistHashBuckets)[hbc];

         if (hlistHashBucket)
         {
            if (! IS_VALID_HANDLE(hlistHashBucket, LIST) ||
                ! WalkList(hlistHashBucket, &IsValidStringWalker, NULL))
               break;
         }
      }

      if (hbc == pcst->hbc)
         bResult = TRUE;
   }

   return(bResult);
}

#endif


/****************************** Public Functions *****************************/

/*
** CreateStringTable()
**
** Creates a new string table.
**
** Arguments:     pcnszt - pointer to NEWSTRINGTABLE descibing string table to
**                          be created
**
** Returns:       Handle to new string table if successful, or NULL if
**                unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CreateStringTable(PCNEWSTRINGTABLE pcnszt,
                                     PHSTRINGTABLE phst)
{
   PSTRINGTABLE pst;

   ASSERT(IS_VALID_STRUCT_PTR(pcnszt, CNEWSTRINGTABLE));
   ASSERT(IS_VALID_WRITE_PTR(phst, HSTRINGTABLE));

   /* Try to allocate new string table structure. */

   *phst = NULL;

   if (AllocateMemory(sizeof(*pst), &pst))
   {
      PHLIST phlistHashBuckets;

      /* Try to allocate hash bucket array. */

#ifdef DBLCHECK
      ASSERT((double)(pcnszt->hbc) * (double)(sizeof(*phlistHashBuckets)) <= (double)SIZE_T_MAX);
#endif

      if (AllocateMemory(pcnszt->hbc * sizeof(*phlistHashBuckets), (PVOID *)(&phlistHashBuckets)))
      {
         HASHBUCKETCOUNT bc;

         /* Successs!  Initialize STRINGTABLE fields. */

         pst->phlistHashBuckets = phlistHashBuckets;
         pst->hbc = pcnszt->hbc;

         /* Initialize all hash buckets to NULL. */

         for (bc = 0; bc < pcnszt->hbc; bc++)
            phlistHashBuckets[bc] = NULL;

         *phst = (HSTRINGTABLE)pst;

         ASSERT(IS_VALID_HANDLE(*phst, STRINGTABLE));
      }
      else
         /* Free string table structure. */
         FreeMemory(pst);
   }

   return(*phst != NULL);
}


/*
** DestroyStringTable()
**
** Destroys a string table.
**
** Arguments:     hst - handle to string table to be destroyed
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DestroyStringTable(HSTRINGTABLE hst)
{
   HASHBUCKETCOUNT bc;

   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));

   /* Traverse array of hash bucket heads, freeing hash bucket strings. */

   for (bc = 0; bc < ((PSTRINGTABLE)hst)->hbc; bc++)
      FreeHashBucket(((PSTRINGTABLE)hst)->phlistHashBuckets[bc]);

   /* Free array of hash buckets. */

   FreeMemory(((PSTRINGTABLE)hst)->phlistHashBuckets);

   /* Free string table structure. */

   FreeMemory((PSTRINGTABLE)hst);

   return;
}


/*
** AddString()
**
** Adds a string to a string table.
**
** Arguments:     pcsz - pointer to string to be added
**                hst - handle to string table that string is to be added to
**
** Returns:       Handle to new string if successful, or NULL if unsuccessful.
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL AddString(LPCTSTR pcsz, HSTRINGTABLE hst, 
                           STRINGTABLEHASHFUNC pfnHashFunc, PHSTRING phs)
{
   BOOL bResult;
   HASHBUCKETCOUNT hbcNew;
   BOOL bFound;
   HNODE hnode;
   PHLIST phlistHashBucket;

   ASSERT(IS_VALID_STRING_PTR(pcsz, CSTR));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
   ASSERT(IS_VALID_CODE_PTR(pfnHashFunc, STRINGTABLEHASHFUNC));
   ASSERT(IS_VALID_WRITE_PTR(phs, HSTRING));

   /* Find appropriate hash bucket. */

   hbcNew = pfnHashFunc(pcsz, ((PSTRINGTABLE)hst)->hbc);

   ASSERT(hbcNew < ((PSTRINGTABLE)hst)->hbc);

   phlistHashBucket = &(((PSTRINGTABLE)hst)->phlistHashBuckets[hbcNew]);

   if (*phlistHashBucket)
   {
      /* Search the hash bucket for the string. */

      bFound = SearchSortedList(*phlistHashBucket, &StringSearchCmp, pcsz,
                                &hnode);
      bResult = TRUE;
   }
   else
   {
      NEWLIST nl;

      /* Create a string list for this hash bucket. */

      bFound = FALSE;

      nl.dwFlags = NL_FL_SORTED_ADD;

      bResult = CreateList(&nl, phlistHashBucket);
   }

   /* Do we have a hash bucket for the string? */

   if (bResult)
   {
      /* Yes.  Is the string already in the hash bucket? */

      if (bFound)
      {
         /* Yes. */

         LockString((HSTRING)hnode);
         *phs = (HSTRING)hnode;
      }
      else
      {
         /* No.  Create it. */

         PSTRING pstringNew;

         /* (+ 1) for null terminator. */

         bResult = AllocateMemory(sizeof(*pstringNew) - sizeof(pstringNew->string)
                                  + (lstrlen(pcsz) + 1) * sizeof(TCHAR), &pstringNew);

         if (bResult)
         {
            HNODE hnodeNew;

            /* Set up STRING fields. */

            pstringNew->ulcLock = 1;
            lstrcpy(pstringNew->string, pcsz);

            /* What's up with this string, Doc? */

            bResult = AddNode(*phlistHashBucket, StringSortCmp, pstringNew, &hnodeNew);

            /* Was the new string added to the hash bucket successfully? */

            if (bResult)
               /* Yes. */
               *phs = (HSTRING)hnodeNew;
            else
               /* No. */
               FreeMemory(pstringNew);
         }
      }
   }

   ASSERT(! bResult ||
          IS_VALID_HANDLE(*phs, STRING));

   return(bResult);
}


/*
** DeleteString()
**
** Decrements a string's lock count.  If the lock count goes to 0, the string
** is deleted from its string table.
**
** Arguments:     hs - handle to the string to be deleted
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void DeleteString(HSTRING hs)
{
   PSTRING pstring;

   ASSERT(IS_VALID_HANDLE(hs, STRING));

   pstring = (PSTRING)GetNodeData((HNODE)hs);

   /* Delete string completely? */

   if (! UnlockString(pstring))
   {
      /* Yes.  Remove the string node from the hash bucket's list. */

      DeleteNode((HNODE)hs);

      FreeMemory(pstring);
   }

   return;
}


/*
** LockString()
**
** Increments a string's lock count.
**
** Arguments:     hs - handle to string whose lock count is to be incremented
**
** Returns:       void
**
** Side Effects:  none
*/
PUBLIC_CODE void LockString(HSTRING hs)
{
   PSTRING pstring;

   ASSERT(IS_VALID_HANDLE(hs, STRING));

   /* Increment lock count. */

   pstring = (PSTRING)GetNodeData((HNODE)hs);

   ASSERT(pstring->ulcLock < ULONG_MAX);
   pstring->ulcLock++;

   return;
}


/*
** CompareStrings()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE COMPARISONRESULT CompareStringsI(HSTRING hs1, HSTRING hs2)
{
   ASSERT(IS_VALID_HANDLE(hs1, STRING));
   ASSERT(IS_VALID_HANDLE(hs2, STRING));

   /* This comparison works across string tables. */

   return(MapIntToComparisonResult(lstrcmpi(((PCSTRING)GetNodeData((HNODE)hs1))->string,
                                            ((PCSTRING)GetNodeData((HNODE)hs2))->string)));
}


/*
** GetString()
**
** Retrieves a pointer to a string in a string table.
**
** Arguments:     hs - handle to the string to be retrieved
**
** Returns:       Pointer to string.
**
** Side Effects:  none
*/
PUBLIC_CODE LPCTSTR GetString(HSTRING hs)
{
   PSTRING pstring;

   ASSERT(IS_VALID_HANDLE(hs, STRING));

   pstring = (PSTRING)GetNodeData((HNODE)hs);

   return((LPCTSTR)&(pstring->string));
}


/*
** WriteStringTable()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT WriteStringTable(HCACHEDFILE hcf, HSTRINGTABLE hst)
{
   TWINRESULT tr = TR_BRIEFCASE_WRITE_FAILED;
   DWORD dwcbStringTableDBHeaderOffset;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));

   /* Save initial file poisition. */

   dwcbStringTableDBHeaderOffset = GetCachedFilePointerPosition(hcf);

   if (dwcbStringTableDBHeaderOffset != INVALID_SEEK_POSITION)
   {
      STRINGTABLEDBHEADER stdbh;

      /* Leave space for the string table header. */

      ZeroMemory(&stdbh, sizeof(stdbh));

      if (WriteToCachedFile(hcf, (PCVOID)&stdbh, sizeof(stdbh), NULL))
      {
         HASHBUCKETCOUNT hbc;

         /* Save strings in each hash bucket. */

         stdbh.dwcbMaxStringLen = 0;
         stdbh.lcStrings = 0;

         tr = TR_SUCCESS;

         for (hbc = 0; hbc < ((PSTRINGTABLE)hst)->hbc; hbc++)
         {
            LONG lcStringsInHashBucket;
            DWORD dwcbStringLen;

            tr = WriteHashBucket(hcf,
                              (((PSTRINGTABLE)hst)->phlistHashBuckets)[hbc],
                              &lcStringsInHashBucket, &dwcbStringLen);

            if (tr == TR_SUCCESS)
            {
               /* Watch out for overflow. */

               ASSERT(stdbh.lcStrings <= LONG_MAX - lcStringsInHashBucket);

               stdbh.lcStrings += lcStringsInHashBucket;

               if (dwcbStringLen > stdbh.dwcbMaxStringLen)
                  stdbh.dwcbMaxStringLen = dwcbStringLen;
            }
            else
               break;
         }

         if (tr == TR_SUCCESS)
         {
            /* Save string table header. */

            // The on-disk dwCBMaxString len always refers to ANSI chars,
            // whereas in memory it is for the TCHAR type, we adjust it
            // around the save

            stdbh.dwcbMaxStringLen /= sizeof(TCHAR);

            tr = WriteDBSegmentHeader(hcf, dwcbStringTableDBHeaderOffset,
                                      &stdbh, sizeof(stdbh));
            
            stdbh.dwcbMaxStringLen *= sizeof(TCHAR);

            TRACE_OUT((TEXT("WriteStringTable(): Wrote %ld strings."),
                       stdbh.lcStrings));
         }
      }
   }

   return(tr);
}


/*
** ReadStringTable()
**
**
**
** Arguments:
**
** Returns:       TWINRESULT
**
** Side Effects:  none
*/
PUBLIC_CODE TWINRESULT ReadStringTable(HCACHEDFILE hcf, HSTRINGTABLE hst,
                                         PHHANDLETRANS phhtTrans)
{
   TWINRESULT tr;
   STRINGTABLEDBHEADER stdbh;
   DWORD dwcbRead;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
   ASSERT(IS_VALID_WRITE_PTR(phhtTrans, HHANDLETRANS));

   if (ReadFromCachedFile(hcf, &stdbh, sizeof(stdbh), &dwcbRead) &&
       dwcbRead == sizeof(stdbh))
   {
      LPTSTR pszStringBuf;

      // The string header will have the ANSI cb max, whereas inmemory
      // we need the cb max based on the current character size

      stdbh.dwcbMaxStringLen *= sizeof(TCHAR);

      if (AllocateMemory(stdbh.dwcbMaxStringLen, &pszStringBuf))
      {
         HHANDLETRANS hht;

         if (CreateHandleTranslator(stdbh.lcStrings, &hht))
         {
            LONG lcStrings;

            tr = TR_SUCCESS;

            TRACE_OUT((TEXT("ReadStringTable(): Reading %ld strings, maximum length %lu."),
                       stdbh.lcStrings,
                       stdbh.dwcbMaxStringLen));

            for (lcStrings = 0;
                 lcStrings < stdbh.lcStrings && tr == TR_SUCCESS;
                 lcStrings++)
               tr = ReadString(hcf, hst, hht, pszStringBuf, stdbh.dwcbMaxStringLen);

            if (tr == TR_SUCCESS)
            {
               PrepareForHandleTranslation(hht);
               *phhtTrans = hht;

               ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));
               ASSERT(IS_VALID_HANDLE(*phhtTrans, HANDLETRANS));
            }
            else
               DestroyHandleTranslator(hht);
         }
         else
            tr = TR_OUT_OF_MEMORY;

         FreeMemory(pszStringBuf);
      }
      else
         tr = TR_OUT_OF_MEMORY;
   }
   else
      tr = TR_CORRUPT_BRIEFCASE;

   return(tr);
}


#if defined(DEBUG) || defined (VSTF)

/*
** IsValidHSTRING()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHSTRING(HSTRING hs)
{
   BOOL bResult;

   if (IS_VALID_HANDLE((HNODE)hs, NODE))
      bResult = IS_VALID_STRUCT_PTR((PSTRING)GetNodeData((HNODE)hs), CSTRING);
   else
      bResult = FALSE;

   return(bResult);
}


/*
** IsValidHSTRINGTABLE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHSTRINGTABLE(HSTRINGTABLE hst)
{
   return(IS_VALID_STRUCT_PTR((PSTRINGTABLE)hst, CSTRINGTABLE));
}

#endif


#ifdef DEBUG

/*
** GetStringCount()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE ULONG GetStringCount(HSTRINGTABLE hst)
{
   ULONG ulcStrings = 0;
   HASHBUCKETCOUNT hbc;

   ASSERT(IS_VALID_HANDLE(hst, STRINGTABLE));

   for (hbc = 0; hbc < ((PCSTRINGTABLE)hst)->hbc; hbc++)
   {
      HLIST hlistHashBucket;

      hlistHashBucket = (((PCSTRINGTABLE)hst)->phlistHashBuckets)[hbc];

      if (hlistHashBucket)
      {
         ASSERT(ulcStrings <= ULONG_MAX - GetNodeCount(hlistHashBucket));
         ulcStrings += GetNodeCount(hlistHashBucket);
      }
   }

   return(ulcStrings);
}

#endif

