//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       subobj.h
//  Content:    This file contains the subobject (our own object)
//              storage space and manipulation mechanisms.
//
//  History:
//  01-17-94 ScottH      Created
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifndef _SUBOBJ_H_
#define _SUBOBJ_H_

// This structure is used as the LPITEMIDLIST that
// the shell uses to identify objects in a folder.  The
// first two bytes are required to indicate the size,
// the rest of the data is opaque to the shell.
typedef struct _SUBOBJ
    {
    USHORT  cbSize;             // Size of this struct
    UINT    uFlags;             // One of SOF_ values
    int     nIconIndex;         // Icon index (in resource)
    struct _SUBOBJ FAR * psoNext;
    char    szName[1];          // Display name
    } SUBOBJ, FAR * PSUBOBJ;

// SUBOBJ flags
//
#define SOF_REMOTE      0x0000      // Remote connectoid
#define SOF_NEWREMOTE   0x0001      // New connection
#define SOF_MEMBER      0x0002      // Subobject is part of object space

//  LPCSTR Subobj_GetName(PSUBOBJ pso);
//
//   Gets the subobject name.
//
#define Subobj_GetName(pso)     ((pso)->szName)

//  UINT Subobj_GetFlags(PSUBOBJ pso);
//
//   Gets the subobject flags.
//
#define Subobj_GetFlags(pso)     ((pso)->uFlags)


// Some other Subobj functions...
//
BOOL    PUBLIC Subobj_New(PSUBOBJ FAR * ppso, LPCSTR pszName, int nIconIndex, UINT uFlags);
void    PUBLIC Subobj_Destroy(PSUBOBJ pso);
BOOL    PUBLIC Subobj_Dup(PSUBOBJ FAR * ppso, PSUBOBJ psoArgs);

#ifdef DEBUG

void PUBLIC Subobj_Dump(PSUBOBJ pso);

#endif

typedef struct _SUBOBJSPACE
    {
    PSUBOBJ     psoFirst;
    PSUBOBJ     psoLast;
    
    int         cItems;

    } SUBOBJSPACE, FAR * PSUBOBJSPACE;

//  PSUBOBJ Sos_FirstItem(void);
//
//   Returns the first object in the subobject space.  NULL if empty.
//
#define Sos_FirstItem()         (g_sos.psoFirst)

//  PSUBOBJ Sos_NextItem(PSUBOBJ pso);
//
//   Returns the next object in the subobject space.  NULL if no more 
//   objects.
//
#define Sos_NextItem(pso)       (pso ? pso->psoNext : NULL)


// Other subobject space functions
//
PSUBOBJ PUBLIC Sos_FindItem(LPCSTR pszName);
BOOL    PUBLIC Sos_AddItem(PSUBOBJ pso);
USHORT  PUBLIC Sos_GetMaxSize(void);
PSUBOBJ PUBLIC Sos_RemoveItem(LPCSTR pszName);
void    PUBLIC Sos_Destroy(void);
int     PUBLIC Sos_FillFromAddressBook(void);
BOOL    PUBLIC Sos_Init(void);
HRESULT PUBLIC Sos_AddRef(void);
void    PUBLIC Sos_Release(void);

#ifdef DEBUG

void PUBLIC Sos_DumpList(void);

#endif

extern SUBOBJSPACE g_sos;

//
// Other prototypes...
//

HRESULT PUBLIC RemObj_CreateInstance(LPSHELLFOLDER psf, UINT cidl, LPCITEMIDLIST FAR * ppidl, REFIID riid, LPVOID FAR * ppvOut);

void PUBLIC Remote_GenerateEvent(LONG lEventId, PSUBOBJ pso, PSUBOBJ psoNew);
BOOL PUBLIC Remote_RenameObject(HWND hwndParent, PSUBOBJ pso, PSUBOBJ FAR * psoNew, LPSTR pszNewName);
BOOL PUBLIC Remote_Dial(HWND hWnd, LPCSTR szEntryName);
BOOL PUBLIC Remote_DeleteObject(PSUBOBJ pso);
HRASCONN PUBLIC Remote_GetConnHandle(LPCSTR szEntryName);

void NEAR PASCAL ConfirmConnection (HWND hwnd, LPSTR szEntry);

STDMETHODIMP CDataObj_CreateInstance(
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST FAR * ppidl,
    IDataObject **ppdtobj);

STDMETHODIMP CDataObj_DidDragDrop(IDataObject *pdtobj, DWORD dwEffect);

STDMETHODIMP CIDLDropTarget_Create(
    HWND hwndOwner,
    LPCITEMIDLIST pidl,
    LPDROPTARGET *ppdropt);

#endif // _SUBOBJ_H_
