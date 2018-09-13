//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       subobj.c
//  Content:    This file contains all the functions for Remote folder's
//              objects.
//  History:
//      01-26-93 SatoNa     Created
//      Tue 23-Feb-1993 14:08:25  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include "contain.h"
#include "subobj.h"
#include "connent.h"

// This is our subobject storage while we are loaded in memory
//
#pragma data_seg("SHAREDATA")
SUBOBJSPACE g_sos = {NULL, NULL, 0};
int g_cSosRef = 0;
#pragma data_seg()

#define CB_SUBOBJ_HEADER        _IOffset(SUBOBJ, szName)


#pragma data_seg(DATASEG_READONLY)
char const c_szRunDLLDial[]   = "RNAUI.DLL,RnaDial ";

ErrTbl const c_Rename[3] = {
        { ERROR_ALREADY_EXISTS,       IDS_ERR_NAME_EXIST },
        { ERROR_INVALID_NAME,         IDS_ERR_INVALID_RENAME },
        { ERROR_OUTOFMEMORY,          IDS_OOM },
        };

ErrTbl const c_Import[3] = {
        { ERROR_CORRUPT_PHONEBOOK,    IDS_ERR_CORRUPT_IMPORT },
        { ERROR_DEVICE_DOES_NOT_EXIST,IDS_ERR_NO_DEVICE_INSTALLED },
        { ERROR_OUTOFMEMORY,          IDS_OOM },
        };
#pragma data_seg()

#define MAXDELNAME  64

//---------------------------------------------------------------------------
// Sub object space routines
//---------------------------------------------------------------------------


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Debug code to dump the contents of a subobj
Returns: --
Cond:    --
*/
void PUBLIC Subobj_Dump(
    PSUBOBJ pso)
    {
    if (IsFlagSet(g_uDumpFlags, DF_SUBOBJ))
        {
        if (pso == NULL)
            {
            TRACE_MSG(TF_ALWAYS, "SUBOBJ Cannot dump NULL subobj!");
            return;
            }

        TRACE_MSG(TF_ALWAYS, "SUBOBJ %s", Dbg_SafeStr(Subobj_GetName(pso)));
        TRACE_MSG(TF_ALWAYS, "       Icon: %x   Flags: %#08x", pso->nIconIndex, pso->uFlags);
        if (pso->psoNext)
            {
            TRACE_MSG(TF_ALWAYS, "       Next: %s", Dbg_SafeStr(Subobj_GetName(pso->psoNext)));
            }
        }
    }


#endif // DEBUG

/*----------------------------------------------------------
Purpose: Allocates a subobject

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Subobj_New(
    PSUBOBJ FAR * ppso,     
    LPCSTR pszName,         // Copied into object struct
    int nIconIndex,         // Icon index
    UINT uFlags)            // SOF_ flags
    {
    USHORT cbSize;

    ASSERT(pszName);

    // We always tack on a USHORT null-terminator on the end
    // because we know the remote folder has no hierarchy.
    cbSize = CB_SUBOBJ_HEADER + lstrlen(pszName)+1;
    *ppso = SharedAlloc(cbSize + sizeof(USHORT));
    if (*ppso)
        {
        PSUBOBJ pso = *ppso;

#ifdef DEBUG

        TRACE_MSG(TF_SUBOBJ, "Creating new subobj: %s", Dbg_SafeStr(pszName));

#endif

        // The cbSize field is used strictly by the shell moniker
        // APIs.  To conserve space, we set the size to be the
        // string length, plus some overhead.
        //
        pso->cbSize = cbSize;
        lstrcpy(pso->szName, pszName);
        pso->psoNext = NULL;
        pso->nIconIndex = nIconIndex;
        pso->uFlags = uFlags;
        }

#ifdef DEBUG
    else
        {
        TRACE_MSG(TF_WARNING, "Cannot allocate a new subobj!");
        }
#endif

    return *ppso != NULL;
    }


/*----------------------------------------------------------
Purpose: Copies the contents of one subobject to another.
Returns: --
Cond:    --
*/
void PRIVATE Subobj_Copy(
    PSUBOBJ psoTo,
    PSUBOBJ psoFrom)
    {
    ASSERT(psoTo);
    ASSERT(psoFrom);

    // Don't forget to copy the USHORT null terminator
    BltByte(psoTo, psoFrom, psoFrom->cbSize + sizeof(USHORT));
    // (We do not copy the next pointer)
    psoTo->psoNext = NULL;
    ClearFlag(psoTo->uFlags, SOF_MEMBER);
    }


/*----------------------------------------------------------
Purpose: Duplicates an existing subobject

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Subobj_Dup(
    PSUBOBJ FAR * ppso,
    PSUBOBJ psoArgs)
    {
    ASSERT(ppso);
    ASSERT(psoArgs);

    *ppso = SharedAlloc(psoArgs->cbSize + sizeof(USHORT));
    if (*ppso)
        {
        PSUBOBJ pso = *ppso;

#ifdef DEBUG

        TRACE_MSG(TF_SUBOBJ, "Duplicating subobj: %s", Dbg_SafeStr(Subobj_GetName(psoArgs)));

#endif

        Subobj_Copy(pso, psoArgs);
        }

#ifdef DEBUG
    else
        {
        TRACE_MSG(TF_WARNING, "Cannot allocate another subobj!");
        }
#endif

    return *ppso != NULL;
    }


/*----------------------------------------------------------
Purpose: Destroys (frees) a subobject
Returns: --
Cond:    --
*/
void PUBLIC Subobj_Destroy(
    PSUBOBJ pso)
    {
    ASSERT(pso);

    if (pso)
        {
        ASSERT(pso->psoNext == NULL);       // Prevent memory leaks
        // Cannot destroy a subobject without first removing
        // it from the subobject space.
        ASSERT(IsFlagClear(Subobj_GetFlags(pso), SOF_MEMBER));

        DEBUG_CODE( TRACE_MSG(TF_SUBOBJ, "Destroying subobj: %s", 
                    Subobj_GetName(pso)); )

        SharedFree(&pso);
        }

#ifdef DEBUG
    else
        {
        TRACE_MSG(TF_WARNING, "Cannot destroy NULL subobj!");
        }
#endif

    }


/*----------------------------------------------------------
Purpose: Appends psoNew to pso

Returns: psoNew
Cond:    --
*/
PSUBOBJ PRIVATE Subobj_Add(
    PSUBOBJ pso,
    PSUBOBJ psoNew)     
    {
    ASSERT(pso);
    ASSERT(psoNew);
    ASSERT(psoNew->psoNext == NULL);

    psoNew->psoNext = pso->psoNext;
    pso->psoNext = psoNew;
    SetFlag(psoNew->uFlags, SOF_MEMBER);

    return psoNew;
    }


/*----------------------------------------------------------
Purpose: Removes psoDel from list

Returns: Ptr to node following psoDel
Cond:    --
*/
PSUBOBJ PRIVATE Subobj_Remove(
    PSUBOBJ * ppsoNext,     // Pointer to psoNext field of previous node
    PSUBOBJ psoDel)
    {
    PSUBOBJ psoRet; 

    ASSERT(psoDel);

    psoRet = psoDel->psoNext;
    if (ppsoNext)
        *ppsoNext = psoRet;
    psoDel->psoNext = NULL;
    ClearFlag(psoDel->uFlags, SOF_MEMBER);

    return psoRet;
    }


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Debug code to dump the subobj space 
Returns: --
Cond:    --
*/
void PUBLIC Sos_DumpList(void)
    {
    PSUBOBJ pso;

    ENTEREXCLUSIVE()
        {
        if (IsFlagSet(g_uDumpFlags, DF_SPACE))
            {
            UINT uFlagsSav = g_uDumpFlags;

            SetFlag(g_uDumpFlags, DF_SUBOBJ);

            pso = g_sos.psoFirst;
            while (pso)
                {
                Subobj_Dump(pso);

                pso = pso->psoNext;
                }

            g_uDumpFlags = uFlagsSav;
            }
        }
    LEAVEEXCLUSIVE()
    }

#endif // DEBUG


/*----------------------------------------------------------
Purpose: Finds a subobject in the subobject space, given the
         name.

Returns: Ptr to the subobject
         NULL if not found
Cond:    --
*/
PSUBOBJ PUBLIC Sos_FindItem(
    LPCSTR pszName)
    {
    PSUBOBJ pso;

    ASSERT(pszName);

    ENTEREXCLUSIVE()
        {
        pso = g_sos.psoFirst;
        while (pso)
            {
            if (lstrcmp(Subobj_GetName(pso), pszName) == 0)
                break;      // Found it

            pso = pso->psoNext;
            }
        }
    LEAVEEXCLUSIVE()

    return pso;
    }
    

/*----------------------------------------------------------
Purpose: Adds a unique pso to the subobject space.  If the subobject
         already exists, then nothing is added and this function
         returns FALSE.

Returns: TRUE if the object is unique and added
Cond:    --
*/
BOOL PUBLIC Sos_AddItem(
    PSUBOBJ pso)
    {
    BOOL bRet;

    ASSERT(pso);

    ENTEREXCLUSIVE()
        {
        // Check if the subobject already exists under this same name
        //
        if (Sos_FindItem(Subobj_GetName(pso)))
            bRet = FALSE;
        else
            {
            // Add the subobject
            //
            if (g_sos.psoLast == NULL)
                {
                g_sos.psoFirst = g_sos.psoLast = pso;
                }
            else
                {
                g_sos.psoLast = Subobj_Add(g_sos.psoLast, pso);
                }

            TRACE_MSG(TF_SUBOBJ, "SOS Added: %s", Subobj_GetName(pso));
            DEBUG_CODE( Subobj_Dump(pso); )

            g_sos.cItems++;
            bRet = TRUE;
            }
        }
    LEAVEEXCLUSIVE()

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Removes a subobject from the list.  However, this function
         does NOT destroy the subobj.

Returns: Ptr to subobj that is removed 
         NULL if not found
Cond:    --
*/
PSUBOBJ PUBLIC Sos_RemoveItem(
    LPCSTR pszName)
    {
    PSUBOBJ pso;

    ENTEREXCLUSIVE()
        {
        PSUBOBJ * ppsoNext;

        pso = g_sos.psoFirst;
        ppsoNext = &g_sos.psoFirst;
        while (pso)
            {
            if (0 == lstrcmp(Subobj_GetName(pso), pszName))
                {
                // Remove the subobject
                PSUBOBJ psoFollow = Subobj_Remove(ppsoNext, pso);
                g_sos.cItems--;

                ASSERT(g_sos.cItems >= 0);

                TRACE_MSG(TF_SUBOBJ, "SOS Removed: %s", Subobj_GetName(pso));

                // Is the last node being removed?
                if (g_sos.psoLast == pso)
                    {
                    // Yes; update the last-node pointer
                    g_sos.psoLast = (PSUBOBJ)((LPSTR)ppsoNext - _IOffset(SUBOBJ, psoNext));
                    }
                break;
                }

            ppsoNext = &pso->psoNext;
            pso = pso->psoNext;
            }
        }
    LEAVEEXCLUSIVE()

    return pso;
    }
    

/*----------------------------------------------------------
Purpose: Returns the size of the subobject that is the biggest.

Returns: count of bytes
Cond:    --
*/
USHORT PUBLIC Sos_GetMaxSize(void)
    {
    PSUBOBJ pso;
    USHORT cbMac = 0;

    ENTEREXCLUSIVE()
        {
        pso = g_sos.psoFirst;
        while (pso)
            {
            cbMac = max(cbMac, pso->cbSize);
            pso = pso->psoNext;
            }
        }
    LEAVEEXCLUSIVE()

    return cbMac + sizeof(USHORT);      // include the USHORT null terminator
    }


/*----------------------------------------------------------
Purpose: Clear out the subobject space.

Returns: --
Cond:    --
*/
void PRIVATE Sos_Clear(void)
    {
    PSUBOBJ pso;
    PSUBOBJ psoNext;

    TRACE_MSG(TF_SUBOBJ, "SOS: Clearing");

    ENTEREXCLUSIVE()
        {
        pso = g_sos.psoFirst;
        while (pso)
            {
            psoNext = Subobj_Remove(NULL, pso);
            Subobj_Destroy(pso);

            pso = psoNext;
            }

        g_sos.psoFirst = g_sos.psoLast = NULL;
        g_sos.cItems = 0;
        }
    LEAVEEXCLUSIVE()
    }


/*----------------------------------------------------------
Purpose: Destroy and free the entire subobject list
Returns: --
Cond:    --
*/
void PUBLIC Sos_Destroy(void)
    {
    Sos_Clear();
    }


/*----------------------------------------------------------
Purpose: Get the connection entries from the address book
         and add them to the subobj space.

Returns: The number of connection entries added
Cond:    --
*/
int PRIVATE Sos_FillFromAddressBook(void)
    {
    PSTR pszAllDevs;
    DWORD cEntries;
    UINT uLen;
    LPSTR pStart;
    PSUBOBJ pso;
    int  cItems = 0;
    int cErrs = 0;

    // Get the number of items
    //
    if ((RnaEnumConnEntries(NULL, 0, &cEntries) != SUCCESS) ||
        cEntries == 0)
        {
        return (int)cEntries;
        }

    TRACE_MSG(TF_SUBOBJ, "SOS Filling from address book");

    // Allocate a buffer to get all the connection friendly
    //  names.  The buffer is a double null-terminated string.
    //
    cEntries *= MAXPATHLEN;
    uLen = (UINT)cEntries+1;

    if ((pszAllDevs = GAllocArray(char, uLen)) == NULL)
        {
        cErrs++;    // Out of memory
        }
    else
        {
        RnaEnumConnEntries(pszAllDevs, uLen, &cEntries);

        for (pStart = pszAllDevs; cEntries > 0; pStart += lstrlen(pStart)+1, cEntries--)
            {
            if (lstrcmpi(pStart, c_szDirect))
            {
                if (!Subobj_New(&pso, pStart, IDI_REMOTE - IDI_ICON, 0))
                    {
                    cErrs++;        // Out of memory
                    }
                else
                    {
                    cItems++;
                    Sos_AddItem(pso);
                    }
                }
            }

        GFree(pszAllDevs);
        }

    if (cErrs > 0)
        {
        // BUGBUG: need a hwnd other than NULL
        MsgBox_Err(NULL, IDS_OOM_FILLSPACE, IDS_CAP_REMOTE);
        }

    return cItems;
    }



/*----------------------------------------------------------
Purpose: Initialize our subobject space.

         We initialize by adding the default objects that
         are always in the window.

Returns: TRUE on success
         FALSE on failure

Cond:    --
*/
BOOL PUBLIC Sos_Init(void)
    {
    BOOL bRet = FALSE;
    PSUBOBJ pso;
    char szName[MAXPATHLEN];

    TRACE_MSG(TF_SUBOBJ, "SOS Initializing");

    Sos_Clear();

    LoadString(ghInstance, IDS_NEWREMOTE, szName, sizeof(szName));
    if (Subobj_New(&pso, szName, IDI_NEWREMOTE - IDI_ICON, SOF_NEWREMOTE))
        {
        Sos_AddItem(pso);
        Sos_FillFromAddressBook();
        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Increments the reference count of the subobject space.

Returns: standard
Cond:    --
*/
HRESULT PUBLIC Sos_AddRef(void)
    {
    HRESULT hres = NOERROR;

    ENTEREXCLUSIVE()
        {
        if (g_cSosRef++ == 0)
            {
            if (!Sos_Init())
                {
                // Failed
                g_cSosRef--;
                hres = ResultFromScode(E_OUTOFMEMORY);
                }
            }
        }
    LEAVEEXCLUSIVE()

    return hres;
    }


/*----------------------------------------------------------
Purpose: Decrements the reference count of the subobject space.

Returns: standard
Cond:    --
*/
void PUBLIC Sos_Release(void)
    {
    ENTEREXCLUSIVE()
        {
        ASSERT(g_cSosRef != 0);

        if (--g_cSosRef == 0)
            {
            Sos_Destroy();
            }
        }
    LEAVEEXCLUSIVE()
    }



//---------------------------------------------------------------------------
// PUBLIC APIS
//---------------------------------------------------------------------------



/*----------------------------------------------------------
Purpose: Send a notification to the shell
Returns: --
Cond:    --
*/
void PUBLIC Remote_GenerateEvent(
    LONG lEventId,      // An SHCNE_ event
    PSUBOBJ pso,
    PSUBOBJ psoNew)     // Used for the SHCNE_RENAMEITEM, NULL otherwise
    {
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlFolder;

    ENTEREXCLUSIVE()
        {
        pidlFolder = g_pidlRemote;
        }
    LEAVEEXCLUSIVE()

    if (pidlFolder == NULL)
        return;

    ASSERT((psoNew != NULL && lEventId == SHCNE_RENAMEITEM) || 
           lEventId != SHCNE_RENAMEITEM);

    pidl = ILCombine(pidlFolder, (LPITEMIDLIST)pso);
    if (pidl)
        {
        if (psoNew)
            {
            LPITEMIDLIST pidlNew = ILCombine(pidlFolder, (LPITEMIDLIST)psoNew);
            if (pidlNew)
                {
                SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, pidlNew);
                ILFree(pidlNew);
                }
            }
        else
            {
            SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, NULL);
            }
        SHChangeNotifyHandleEvents();
        ILFree(pidl);
        }
    }

/*----------------------------------------------------------
Purpose: Rename a subobject and its corresponding address
         book entry.

Returns: TRUE on success
         FALSE on failure
Cond:    --
*/
BOOL PUBLIC Remote_RenameObject(
    HWND hwndParent,
    PSUBOBJ pso,
    PSUBOBJ FAR * ppsoNew,
    LPSTR pszNewName)      // BUGBUG: this should be LPCSTR
    {
    BOOL bRet = FALSE;
    DWORD dwRet;

    // "New Connection" should have been blocked before this
    ASSERT(IsFlagClear(Subobj_GetFlags(pso), SOF_NEWREMOTE));

    // Check the new name length
    //
    if (lstrlen(pszNewName) > RAS_MaxEntryName)
    {
      MsgBox_Err(hwndParent, IDS_ERR_NAME_TOO_LONG, IDS_CAP_REMOTE);
      return FALSE;
    };

    // Rename the entry in the address book
    if ((dwRet = RnaRenameConnEntry(Subobj_GetName(pso), pszNewName)) == SUCCESS)
        {
        PSUBOBJ psoNew;

        // Create a subobject with the new name
        if (!Subobj_New(&psoNew, pszNewName, IDI_REMOTE, 0))
            {
            // BUGBUG: out of memory, show message box
            }
        else
            {
            // Add the new subobj to the space
            Sos_AddItem(psoNew);
            *ppsoNew = psoNew;

            // Notify shell of a change in the container
            Remote_GenerateEvent(SHCNE_RENAMEITEM, pso, psoNew);

            // Delete the old name
            Sos_RemoveItem(Subobj_GetName(pso));
            Subobj_Destroy(pso);
            bRet = TRUE;
            }
        }
    else
        {
        ETMsgBox(hwndParent, IDS_CAP_REMOTE, dwRet, c_Rename, ARRAYSIZE(c_Rename));
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Delete the object from the subobject space and from
         the address book.

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Remote_DeleteObject(
    PSUBOBJ pso)
    {
    BOOL bRet = FALSE;

    ASSERT(pso);

    // Remove the object from the remote engine
    //
    if (RnaDeleteConnEntry(Subobj_GetName(pso)))
        {
        PSUBOBJ psoDel = Sos_RemoveItem(Subobj_GetName(pso));

        // Notify shell of a change in the container
        //
        Remote_GenerateEvent(SHCNE_DELETE, psoDel, NULL);

        Subobj_Destroy(psoDel);

        bRet = TRUE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get the active connection handle.

Returns: The active connection handle or NULL.
Cond:    --
*/
HRASCONN PUBLIC Remote_GetConnHandle(
    LPCSTR szEntryName)
    {
    LPRASCONN lpRasConn;
    DWORD     cb, cEntries;
    HRASCONN  hrasconn;
    UINT      i;

    // Get the connection handle
    //
    hrasconn = NULL;
    cb = 0;
    if (RasEnumConnections(NULL, &cb, &cEntries) == ERROR_BUFFER_TOO_SMALL)
      {
      if ((cEntries > 0) && (cb > 0))
        {
        if ((lpRasConn = (LPRASCONN)LocalAlloc(LMEM_FIXED, cb)) != NULL)
          {
          lpRasConn->dwSize = sizeof(RASCONN);
          if (RasEnumConnections(lpRasConn, &cb, &cEntries) == ERROR_SUCCESS)
            {
            for (i = 0; i < cEntries; i++)
              {
              if (!lstrcmpi(lpRasConn[i].szEntryName, szEntryName))
                {
                hrasconn = lpRasConn[i].hrasconn;
                break;
                }
              }
            }
          LocalFree(lpRasConn);
          }
        }
      }
    return hrasconn;
    }


/*----------------------------------------------------------
Purpose: Dial the modem

Returns: TRUE on success

Cond:    --
*/
BOOL PUBLIC Remote_Dial(
    HWND hWnd, 
    LPCSTR szEntryName)
    {
    LPSTR   lpszCmdLine;
    BOOL    bResult;

    // Check the entry name
    if (RnaValidateEntryName((LPSTR)szEntryName, FALSE) != ERROR_SUCCESS)
    {
      MsgBox_Err(hWnd, IDS_ERR_INVALID_CONN, IDS_CAP_REMOTE);
      return FALSE;
    };

    // Allocate the command line buffer
    if ((lpszCmdLine = (LPSTR)LocalAlloc(LMEM_FIXED, sizeof(c_szRunDLLDial)+
                                         lstrlen(szEntryName))) == NULL)
        MsgBox_Err(hWnd, IDS_OOM_FILLSPACE, IDS_CAP_REMOTE);

    // Construct the command line
    lstrcpy(lpszCmdLine, c_szRunDLLDial);
    lstrcat(lpszCmdLine, szEntryName);

    // Start the dialing thread
    bResult = RunDLLProcess(lpszCmdLine);

    // Free the buffer
    LocalFree((HLOCAL)lpszCmdLine);
    return bResult;
    }

/*----------------------------------------------------------
Purpose: Dial the modem

Returns: TRUE on success

Cond:    --
*/
void WINAPI RnaDial (HWND hWnd,
                     HINSTANCE hAppInstance,
                     LPSTR lpszCmdLine,
                     int   nCmdShow)
    {
    if (RnaUIDial(hWnd, lpszCmdLine) == ERROR_SUCCESS)
      {
      ConfirmConnection(hWnd, lpszCmdLine);
      };
    return;
    }

/*----------------------------------------------------------
Purpose: Run Rna imported file

Returns: TRUE on success

Cond:    --
*/
void WINAPI RnaRunImport (HWND hWnd,
                          HINSTANCE hAppInstance,
                          LPSTR lpszCmdLine,
                          int   nCmdShow)
    {
    char    szEntryName[RAS_MaxEntryName+1];
    DWORD   dwRet;

    // First import the file
    //
    dwRet = RnaImportEntry(lpszCmdLine, szEntryName, sizeof(szEntryName));

    switch (dwRet)
    {
      case ERROR_SUCCESS:
      {
        PSUBOBJ pso;

        // Create a new subobject with no name
        //
        if (Subobj_New(&pso, szEntryName, IDI_REMOTE, 0))
        {
          // Notify the event
          //
          Remote_GenerateEvent(SHCNE_CREATE, pso, NULL);
          Subobj_Destroy(pso);
        };
      }

      //
      // Fall through!! Fall through!! Fall through!! Fall through!!
      // ===========================================================
      //
      // We created a new entry successfully or it already exists
      // Just dial the connection.
      //
      // ===========================================================
      // Fall through!! Fall through!! Fall through!! Fall through!!
      //
      case ERROR_ALREADY_EXISTS:
        RnaDial(hWnd, hAppInstance, szEntryName, nCmdShow);
        break;

      default:
        ETMsgBox(hWnd, IDS_CAP_REMOTE, dwRet, c_Import, ARRAYSIZE(c_Import));
        break;
    };

    return;
    }

/*----------------------------------------------------------
Purpose: Notify the entry

Returns: TRUE on success

Cond:    --
*/
void WINAPI Remote_Notify (LPSTR szEntryName)
    {
    PSUBOBJ pso;

    // Update the remote folder
    //
    if (Subobj_New(&pso, szEntryName, IDI_REMOTE, 0))
      {
      // Notify the event
      //
      Remote_GenerateEvent(SHCNE_UPDATEITEM, pso, NULL);
      Subobj_Destroy(pso);
      };
    return;
    }


//---------------------------------------------------------------------------
// RemObj Class
//---------------------------------------------------------------------------


typedef struct _RemObj
    {
    // We use the xi also as our IUnknown interface
    IExtractIcon        xi;             // 1st base class
    IContextMenu        ctm;            // 2nd base class
    UINT                cRef;           // reference count
    LPSHELLFOLDER       psf;            
    UINT                cso;            // count of subobjects
    PSUBOBJ             rgpso[1];       // array of subobjects
    } RemObj, * PREMOBJ;


//---------------------------------------------------------------------------
// RemObj IUnknown base member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP RemObj_QueryInterface(
    LPUNKNOWN punk, 
    REFIID riid, 
    LPVOID FAR* ppvOut)
    {
    PREMOBJ this = IToClass(RemObj, xi, punk);
    HRESULT hres = ResultFromScode(E_NOINTERFACE);

    if (IsEqualIID(riid, &IID_IUnknown) || 
        IsEqualIID(riid, &IID_IExtractIcon))
        {
        // We use the xi field as our IUnknown as well
        *ppvOut = &this->xi;
        this->cRef++;
        hres = NOERROR;
        }
    else if (IsEqualIID(riid, &IID_IContextMenu))
        {
        (LPCONTEXTMENU)*ppvOut = &this->ctm;
        this->cRef++;
        hres = NOERROR;
        }
        
    return hres;
    }


/*----------------------------------------------------------
Purpose: IUnknown::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemObj_AddRef(
    LPUNKNOWN punk)
    {
    PREMOBJ this = IToClass(RemObj, xi, punk);

    return ++this->cRef;
    }


/*----------------------------------------------------------
Purpose: IUnknown::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemObj_Release(
    LPUNKNOWN punk)
    {
    PREMOBJ this = IToClass(RemObj, xi, punk);
    UINT i;

    if (--this->cRef) 
        {
        return this->cRef;
        }

    for (i = 0; i < this->cso; i++)
        {
        if (this->rgpso[i])
            {
            Subobj_Destroy(this->rgpso[i]);
            }
        }

    if (this->psf)
        {
        this->psf->lpVtbl->Release(this->psf);
        }

    ENTEREXCLUSIVE()
        {
        --g_cRef;
        }
    LEAVEEXCLUSIVE()

    LocalFree((HLOCAL)this);
    return 0;
    }


//---------------------------------------------------------------------------
// Remote Object IExtractIcon
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IExtractIcon::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP RemObj_XI_QueryInterface(
    LPEXTRACTICON pxi,
    REFIID riid, 
    LPVOID FAR* ppvOut)
    {
    return RemObj_QueryInterface((LPUNKNOWN)pxi, riid, ppvOut);
    }


/*----------------------------------------------------------
Purpose: IExtractIcon::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemObj_XI_AddRef(
    LPEXTRACTICON pxi)
    {
    return RemObj_AddRef((LPUNKNOWN)pxi);
    }


/*----------------------------------------------------------
Purpose: IExtractIcon::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemObj_XI_Release(
    LPEXTRACTICON pxi)
    {
    return RemObj_Release((LPUNKNOWN)pxi);
    }


/*----------------------------------------------------------
Purpose: IExtractIcon::GetIconLocation

         This member function is called to get the file and
         icon index where the shell can get the icon of a 
         particular subobject.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP RemObj_XI_GetIconLocation(
    IExtractIcon FAR * pei,
    UINT uFlags, 
    LPSTR pszIconFile, 
    UINT cchMax, 
    int  FAR * piIndex, 
    UINT FAR * pwFlags)
    {
    PREMOBJ this = IToClass(RemObj, xi, pei);
    PSUBOBJ pso = this->rgpso[0];
    HRESULT hres = NOERROR;

    ASSERT(pso);

    DBG_ENTER_SZ("RemObj_XI_GetIconLocation", Dbg_SafeStr(Subobj_GetName(pso)));

    if (IsFlagSet(uFlags, GIL_OPENICON))
        {
        hres = ResultFromScode(S_FALSE);
        goto Leave;
        }

#ifdef DEBUG

    TRACE_MSG(TF_GENERAL, "Getting icon for: %s", Dbg_SafeStr(Subobj_GetName(pso)));

#endif

    GetModuleFileName(ghInstance, pszIconFile, cchMax);

    if (IsFlagSet(Subobj_GetFlags(pso), SOF_NEWREMOTE))
        {
        *piIndex = NEWOBJ_ICON;
        }
    else
        {
        *piIndex = CONN_ICON;
        }

    *pwFlags = GIL_PERINSTANCE;

Leave:

    DBG_EXIT_HRES("RemObj_XI_GetIconLocation", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IExtractIcon::ExtractIcon

         We have the shell handle the extraction by returning
         S_FALSE.

Returns: S_FALSE
Cond:    --
*/
STDMETHODIMP RemObj_XI_ExtractIcon(
    LPEXTRACTICON pxi,
    LPCSTR pszFile,
    UINT   nIconIndex,
    HICON  FAR *phiconLarge,
    HICON  FAR *phiconSmall,
    UINT   nIcons)
    {
    return ResultFromScode(S_FALSE);
    }


//---------------------------------------------------------------------------
// RemObj ContextMenu member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IContextMenu::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP RemObj_CM_QueryInterface(
    LPCONTEXTMENU pcm,
    REFIID riid, 
    LPVOID FAR* ppvOut)
    {
    PREMOBJ this = IToClass(RemObj, ctm, pcm);
    return RemObj_QueryInterface((LPUNKNOWN)&this->xi, riid, ppvOut);
    }


/*----------------------------------------------------------
Purpose: IContextMenu::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemObj_CM_AddRef(
    LPCONTEXTMENU pcm)
    {
    PREMOBJ this = IToClass(RemObj, ctm, pcm);
    return RemObj_AddRef((LPUNKNOWN)&this->xi);
    }


/*----------------------------------------------------------
Purpose: IContextMenu::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemObj_CM_Release(
    LPCONTEXTMENU pcm)
    {
    PREMOBJ this = IToClass(RemObj, ctm, pcm);
    return RemObj_Release((LPUNKNOWN)&this->xi);
    }


/*----------------------------------------------------------
Purpose: IContextMenu::QueryContextMenu handler for sub-objects
Returns: 
Cond:    --
*/
STDMETHODIMP RemObj_CM_QueryContextMenu(
    IContextMenu FAR * pcm,
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast, 
    UINT uFlags)
    {
    PREMOBJ this = IToClass(RemObj, ctm, pcm);
    USHORT cItems = 0;
    PSUBOBJ pso = this->rgpso[0];
    ULONG rgf;

    DBG_ENTER("RemObj_CM_QueryContextMenu");

    // Is this not being added to the "File" menu, and
    // is this for real-objects (not links)?
    if (IsFlagClear(uFlags, CMF_DVFILE) &&
        IsFlagClear(uFlags, CMF_VERBSONLY))
        {
        // Yes; add the basic menu in POPUP_CONTEXT to the context menu
        cItems += MergePopupMenu(&hmenu, POPUP_CONTEXT, 2, indexMenu, 
                       idCmdFirst - RSVIDM_FIRST, idCmdLast);
        }

    // Is this the New Connection subobject?
    if (IsFlagSet(Subobj_GetFlags(pso), SOF_NEWREMOTE))
        {
        // Yes; merge the Create menu
        cItems += MergePopupMenu(&hmenu, POPUP_CONTEXT, 0, indexMenu, 
                       idCmdFirst - RSVIDM_FIRST, idCmdLast);
        }
    else
        {
        HRASCONN hrasconn;
        UINT iSubMenu;

        // Determine the menu appearance
        if ((hrasconn = Remote_GetConnHandle(Subobj_GetName(pso)))
            == NULL)
            {
            iSubMenu = 1;
            }
            else
            {
            iSubMenu = 3;
            };

        // No; merge the Connect menu
        cItems += MergePopupMenu(&hmenu, POPUP_CONTEXT, iSubMenu, indexMenu,
                       idCmdFirst - RSVIDM_FIRST, idCmdLast);

        if (hrasconn != NULL)
            {
            RASCONNSTATUS rcstat;

            rcstat.dwSize = sizeof(rcstat);
            if ((RasGetConnectStatus(hrasconn, &rcstat) == ERROR_SUCCESS) &&
                (rcstat.rasconnstate == RASCS_Disconnected))
                {
                EnableMenuItem(hmenu, idCmdFirst + RSVIDM_DISCONNECT,
                               MF_GRAYED);
                };
            };
        }
    SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);

    // Is this for real-objects?  (not links)
    if (IsFlagClear(uFlags, CMF_VERBSONLY))
        {
        // Yes; get the attributes of this subobject to determine which
        // menu items are enabled
        rgf = 0;
        this->psf->lpVtbl->GetAttributesOf(this->psf, this->cso, 
            (LPCITEMIDLIST *)this->rgpso, &rgf);

        EnableMenuItem(hmenu, idCmdFirst + RSVIDM_DELETE,
            IsFlagSet(rgf, SFGAO_CANDELETE) ? MF_ENABLED : MF_GRAYED);

        EnableMenuItem(hmenu, idCmdFirst + RSVIDM_PROPERTIES,
            IsFlagSet(rgf, SFGAO_HASPROPSHEET) ? MF_ENABLED : MF_GRAYED);
        }

    // Are multiple objects selected?
    if (1 < this->cso)
        {
        // Yes; disable default verb item
        EnableMenuItem(hmenu, 0, MF_BYPOSITION | MF_GRAYED);
        }

    DBG_EXIT_US("RemObj_CM_QueryContextMenu", cItems);

    return ResultFromShort(cItems);   // number of menu items
    }


#pragma data_seg(DATASEG_READONLY)
const static struct {
    LPCSTR pszVerb;
    UINT idCmd;
    } rgcmds[] = {
        { c_szConnect,      RSVIDM_CONNECT },
        { c_szCreate,       RSVIDM_CREATE },
        { c_szDelete,       RSVIDM_DELETE },
        { c_szLink,         RSVIDM_LINK },
        { c_szRename,       RSVIDM_RENAME },
        { c_szProperties,   RSVIDM_PROPERTIES } };
#pragma data_seg()

/*----------------------------------------------------------
Purpose: IContextMenu::GetCommandString

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP RemObj_CM_GetCommandString(
    IContextMenu FAR * pcm,
    UINT idCmd, 
    UINT wFlags, 
    UINT FAR * pmf,         // BUGBUG: what is this?
    LPSTR pszName, 
    UINT cchMax)
    {
    HRESULT hres = ResultFromScode(E_FAIL);

    DBG_ENTER("RemObj_CM_GetCommandString");

    switch (wFlags)
        {
    // Validate command id or Getting command string?
    case GCS_VALIDATE:
    case GCS_VERB:
        {
        int i;

        for (i = 0; i < ARRAYSIZE(rgcmds); i++)
            {
            if (rgcmds[i].idCmd == idCmd)
                {
                if (wFlags == GCS_VERB)
                    {
                    lstrcpyn(pszName, rgcmds[i].pszVerb, cchMax);
                    };
                hres = NOERROR;
                break;
                }
            }
        }
        break;

    // Getting help text?
    case GCS_HELPTEXT:
        {
        // Yes
        UINT ids = idCmd + IDS_MH_FIRST;

        if (0 != LoadString(ghInstance, ids, pszName, cchMax))
            {
            hres = NOERROR;
            }
        }
        break;
        };

    DBG_EXIT_HRES("RemObj_CM_GetCommandString", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Get the ID of the invoked menu item.  The ID is within
         the RSVIDM_FIRST/LAST range.

Returns: id
Cond:    --
*/
UINT PRIVATE GetIDCmd(
    LPCSTR pszCmd)
    {
    UINT idCmd = LOWORD(pszCmd);

    if (0 != HIWORD(pszCmd))
        {
        int i;

        idCmd = 0;
        for (i = 0; i < ARRAYSIZE(rgcmds); i++)
            {
            if (IsSzEqual(rgcmds[i].pszVerb, pszCmd))
                {
                idCmd = rgcmds[i].idCmd;
                break;
                }
            }
        }
    return idCmd;
    }


/*----------------------------------------------------------
Purpose: Set the deletion confirmation text

Returns: None

Cond:    --
*/
void NEAR PASCAL InitConfirmDeleteDlg (HWND hDlg, PREMOBJ this)
{
  char  szFmt[MAXPATHLEN];
  char  szBuf[MAXPATHLEN];
  char  szShortName[MAXDELNAME+1];
  DWORD dwArg;

  // Check the number of connections being deleted
  if (this->cso == 1)
  {
    // Only one item is being deleted.
    ShortenName(Subobj_GetName(this->rgpso[0]), szShortName, sizeof(szShortName));
    dwArg = (DWORD)szShortName;
  }
  else
  {
    // Multiple items are being deleted
    dwArg = (DWORD)this->cso;
  }

  // Get the format of the displayed text
  GetDlgItemText(hDlg, IDC_DEL_TEXT, szFmt, sizeof(szFmt));
  FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                 (LPCVOID)szFmt, 0, 0,
                 szBuf, sizeof(szBuf), (va_list*) &dwArg);

  SetDlgItemText(hDlg, IDC_DEL_TEXT, szBuf);
  return;
}

/*----------------------------------------------------------
Purpose: Display the deletion confirmation dialog box

Returns: None

Cond:    --
*/
LONG CALLBACK ConfirmDeleteDlgProc (HWND hDlg, UINT message, WPARAM wParam,
                                    LPARAM lParam)
{
  switch(message)
  {
    case WM_INITDIALOG:
        InitConfirmDeleteDlg(hDlg, (PREMOBJ)lParam);
        return TRUE;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDYES:
            case IDNO:
            case IDCANCEL:
              EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
              break;

            default:
              break;
        }
        break;

    default:
        break;
  };
  return FALSE;
}

/*----------------------------------------------------------
Purpose: IContextMenu::InvokeCommand

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP RemObj_CM_InvokeCommand(
    IContextMenu FAR * pcm,
    LPCMINVOKECOMMANDINFO pici)                                       
    {
    HWND hwnd = pici->hwnd;
    //LPCSTR pszWorkingDir = pici->lpDirectory;
    LPCSTR pszCmd = pici->lpVerb;
    //LPCSTR pszParam = pici->lpParameters;
    //int iShowCmd = pici->nShow;
        
    PREMOBJ this = IToClass(RemObj, ctm, pcm);
    BOOL bResult;
    UINT idCmd = GetIDCmd(pszCmd);    
    UINT i;

    DBG_ENTER("RemObj_CM_InvokeCommand");

    // Determine the action based on the command
    //
    switch(idCmd)
        {
    case RSVIDM_CREATE:
        // Create a new connection
        RunWizard(hwnd, CLIENT_WIZ);
        bResult = TRUE;
        break;

    case RSVIDM_STATUS:
    case RSVIDM_CONNECT:
        // Connect
        bResult = Remote_Dial(hwnd, Subobj_GetName(this->rgpso[0]));
        break;

    case RSVIDM_DISCONNECT:
        {
        HRASCONN hrasconn;

        // Connect
        if ((hrasconn = Remote_GetConnHandle(Subobj_GetName(this->rgpso[0])))
            != NULL)
          {
          RasHangUp(hrasconn);
          };
        bResult = TRUE;
        break;
        }
    case RSVIDM_LINK:
        {
        LPDATAOBJECT pdtobj;

        if (this->psf->lpVtbl->GetUIObjectOf(this->psf, hwnd, this->cso,
                                            (LPCITEMIDLIST *)this->rgpso,
                                            &IID_IDataObject, NULL, &pdtobj)
            == NOERROR)
            {
            SHCreateLinks(hwnd, NULL, pdtobj,
                          SHCL_USEDESKTOP | SHCL_USETEMPLATE | SHCL_CONFIRM,
                          NULL);
            pdtobj->lpVtbl->Release(pdtobj);
            };
        break;
        }
    case RSVIDM_DELETE:
        // Confirm with the user
        if (this->cso > 0)
        {
            if (DialogBoxParam(ghInstance,
                               MAKEINTRESOURCE(this->cso == 1 ?
                               IDD_DELETE_CONN : IDD_DELETE_MULTIPLE),
                               hwnd, ConfirmDeleteDlgProc, (LPARAM)this) != IDYES)
                {
                break;
                };
        }

        for (i = 0; i < this->cso; i++)
            {
            Remote_DeleteObject(this->rgpso[i]);
            }
        break;

    case RSVIDM_PROPERTIES:
        Remote_PropertySheet(Subobj_GetName(this->rgpso[0]), hwnd);
        break;

    default:
        bResult = FALSE;
        break;
        };

    DBG_EXIT("RemObj_CM_InvokeCommand");

    return (bResult ? NOERROR : ResultFromScode(E_FAIL));
    }


//---------------------------------------------------------------------------
// RemObj class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

IExtractIconVtbl c_RemObj_XIVtbl =
    {
    RemObj_XI_QueryInterface,
    RemObj_XI_AddRef,
    RemObj_XI_Release,
    RemObj_XI_GetIconLocation,
    RemObj_XI_ExtractIcon
    };

IContextMenuVtbl c_RemObj_CMVtbl =
    {
    RemObj_CM_QueryInterface,
    RemObj_CM_AddRef,
    RemObj_CM_Release,
    RemObj_CM_QueryContextMenu,
    RemObj_CM_InvokeCommand,
    RemObj_CM_GetCommandString,
    } ;

#pragma data_seg()


/*----------------------------------------------------------
Purpose: Creates an interface for a RemObj.

Returns: standard hresult
Cond:    --
*/
HRESULT PUBLIC RemObj_CreateInstance(
    LPSHELLFOLDER psf,
    UINT cidl,
    LPCITEMIDLIST FAR * ppidl,
    REFIID riid,
    LPVOID FAR * ppvOut)
    {
    HRESULT hres;
    PSUBOBJ * ppso;
    PREMOBJ this;
    UINT i;

    DBG_ENTER_RIID("RemObj_CreateInstance", riid);

    ASSERT(cidl > 0);
    ASSERT(ppidl);
    ASSERT(*ppidl);
    ASSERT(ppvOut);

    this = LocalAlloc(LPTR, sizeof(*this) + (cidl-1)*sizeof(PSUBOBJ));
    if (!this) 
        {
        hres = ResultFromScode(E_OUTOFMEMORY);
        goto Leave;
        }
    this->xi.lpVtbl = &c_RemObj_XIVtbl;
    this->ctm.lpVtbl = &c_RemObj_CMVtbl;
    this->cRef = 1;
    this->cso = cidl;

    for (i = 0, ppso = &this->rgpso[0]; i < cidl; i++, ppso++)
        {
        if (!Subobj_Dup(ppso, (PSUBOBJ)ppidl[i]))
            {
            // Failed; cleanup
            UINT iClean;
            for (iClean = 0, ppso = &this->rgpso[0]; iClean < i; iClean++, ppso++)
                {
                Subobj_Destroy(*ppso);
                }
            LocalFree(this);
            hres = ResultFromScode(E_OUTOFMEMORY);
            goto Leave;
            }
        }

    this->psf = psf;
    this->psf->lpVtbl->AddRef(this->psf);

    ENTEREXCLUSIVE()
        {
        ++g_cRef;
        }
    LEAVEEXCLUSIVE()

    // Note that the Release member will free the object, if 
    // QueryInterface failed.
    hres = this->xi.lpVtbl->QueryInterface(&this->xi, riid, ppvOut);
    this->xi.lpVtbl->Release(&this->xi);

Leave:
    DBG_EXIT_HRES("RemObj_CreateInstance", hres);

    return hres;        // S_OK or E_NOINTERFACE
    }

