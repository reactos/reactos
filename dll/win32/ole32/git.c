/*
 * Implementation of the StdGlobalInterfaceTable object
 *
 * The GlobalInterfaceTable (GIT) object is used to marshal interfaces between
 * threading apartments (contexts). When you want to pass an interface but not
 * as a parameter, it wouldn't get marshalled automatically, so you can use this
 * object to insert the interface into a table, and you get back a cookie.
 * Then when it's retrieved, it'll be unmarshalled into the right apartment.
 *
 * Copyright 2003 Mike Hearn <mike@theoretic.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"

#include "compobj_private.h" 

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/****************************************************************************
 * StdGlobalInterfaceTable definition
 *
 * This class implements IGlobalInterfaceTable and is a process-wide singleton
 * used for marshalling interfaces between threading apartments using cookies.
 */

/* Each entry in the linked list of GIT entries */
typedef struct StdGITEntry
{
  DWORD cookie;
  IID iid;         /* IID of the interface */
  IStream* stream; /* Holds the marshalled interface */

  struct list entry;
} StdGITEntry;

/* Class data */
typedef struct StdGlobalInterfaceTableImpl
{
  const IGlobalInterfaceTableVtbl *lpVtbl;

  ULONG ref;
  struct list list;
  ULONG nextCookie;
  
} StdGlobalInterfaceTableImpl;

void* StdGlobalInterfaceTableInstance;

static CRITICAL_SECTION git_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &git_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": global interface table") }
};
static CRITICAL_SECTION git_section = { &critsect_debug, -1, 0, 0, 0, 0 };


/** This destroys it again. It should revoke all the held interfaces first **/
static void StdGlobalInterfaceTable_Destroy(void* self)
{
  TRACE("(%p)\n", self);
  FIXME("Revoke held interfaces here\n");
  
  HeapFree(GetProcessHeap(), 0, self);
  StdGlobalInterfaceTableInstance = NULL;
}

/***
 * A helper function to traverse the list and find the entry that matches the cookie.
 * Returns NULL if not found. Must be called inside git_section critical section.
 */
static StdGITEntry*
StdGlobalInterfaceTable_FindEntry(IGlobalInterfaceTable* iface, DWORD cookie)
{
  StdGlobalInterfaceTableImpl* const self = (StdGlobalInterfaceTableImpl*) iface;
  StdGITEntry* e;

  TRACE("iface=%p, cookie=0x%x\n", iface, cookie);

  LIST_FOR_EACH_ENTRY(e, &self->list, StdGITEntry, entry) {
    if (e->cookie == cookie)
      return e;
  }
  
  TRACE("Entry not found\n");
  return NULL;
}

/***
 * Here's the boring boilerplate stuff for IUnknown
 */

static HRESULT WINAPI
StdGlobalInterfaceTable_QueryInterface(IGlobalInterfaceTable* iface,
               REFIID riid, void** ppvObject)
{
  /* Make sure silly coders can't crash us */
  if (ppvObject == 0) return E_INVALIDARG;

  *ppvObject = 0; /* assume we don't have the interface */

  /* Do we implement that interface? */
  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_IGlobalInterfaceTable, riid))
    *ppvObject = iface;
  else
    return E_NOINTERFACE;

  /* Now inc the refcount */
  IGlobalInterfaceTable_AddRef(iface);
  return S_OK;
}

static ULONG WINAPI
StdGlobalInterfaceTable_AddRef(IGlobalInterfaceTable* iface)
{
  StdGlobalInterfaceTableImpl* const self = (StdGlobalInterfaceTableImpl*) iface;

  /* InterlockedIncrement(&self->ref); */
  return self->ref;
}

static ULONG WINAPI
StdGlobalInterfaceTable_Release(IGlobalInterfaceTable* iface)
{
  StdGlobalInterfaceTableImpl* const self = (StdGlobalInterfaceTableImpl*) iface;

  /* InterlockedDecrement(&self->ref); */
  if (self->ref == 0) {
    /* Hey ho, it's time to go, so long again 'till next weeks show! */
    StdGlobalInterfaceTable_Destroy(self);
    return 0;
  }

  return self->ref;
}

/***
 * Now implement the actual IGlobalInterfaceTable interface
 */

static HRESULT WINAPI
StdGlobalInterfaceTable_RegisterInterfaceInGlobal(
               IGlobalInterfaceTable* iface, IUnknown* pUnk,
               REFIID riid, DWORD* pdwCookie)
{
  StdGlobalInterfaceTableImpl* const self = (StdGlobalInterfaceTableImpl*) iface;
  IStream* stream = NULL;
  HRESULT hres;
  StdGITEntry* entry;
  LARGE_INTEGER zero;

  TRACE("iface=%p, pUnk=%p, riid=%s, pdwCookie=0x%p\n", iface, pUnk, debugstr_guid(riid), pdwCookie);

  if (pUnk == NULL) return E_INVALIDARG;
  
  /* marshal the interface */
  TRACE("About to marshal the interface\n");

  hres = CreateStreamOnHGlobal(0, TRUE, &stream);
  if (hres) return hres;
  hres = CoMarshalInterface(stream, riid, pUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLESTRONG);
  if (hres)
  {
    IStream_Release(stream);
    return hres;
  }

  zero.QuadPart = 0;
  IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

  entry = HeapAlloc(GetProcessHeap(), 0, sizeof(StdGITEntry));
  if (entry == NULL) return E_OUTOFMEMORY;

  EnterCriticalSection(&git_section);
  
  entry->iid = *riid;
  entry->stream = stream;
  entry->cookie = self->nextCookie;
  self->nextCookie++; /* inc the cookie count */

  /* insert the new entry at the end of the list */
  list_add_tail(&self->list, &entry->entry);

  /* and return the cookie */
  *pdwCookie = entry->cookie;
  
  LeaveCriticalSection(&git_section);
  
  TRACE("Cookie is 0x%x\n", entry->cookie);
  return S_OK;
}

static HRESULT WINAPI
StdGlobalInterfaceTable_RevokeInterfaceFromGlobal(
               IGlobalInterfaceTable* iface, DWORD dwCookie)
{
  StdGITEntry* entry;
  HRESULT hr;

  TRACE("iface=%p, dwCookie=0x%x\n", iface, dwCookie);

  EnterCriticalSection(&git_section);

  entry = StdGlobalInterfaceTable_FindEntry(iface, dwCookie);
  if (entry == NULL) {
    TRACE("Entry not found\n");
    LeaveCriticalSection(&git_section);
    return E_INVALIDARG; /* not found */
  }

  list_remove(&entry->entry);

  LeaveCriticalSection(&git_section);
  
  /* Free the stream */
  hr = CoReleaseMarshalData(entry->stream);
  if (hr != S_OK)
  {
    WARN("Failed to release marshal data, hr = 0x%08x\n", hr);
    return hr;
  }
  IStream_Release(entry->stream);
		    
  HeapFree(GetProcessHeap(), 0, entry);
  return S_OK;
}

static HRESULT WINAPI
StdGlobalInterfaceTable_GetInterfaceFromGlobal(
               IGlobalInterfaceTable* iface, DWORD dwCookie,
               REFIID riid, void **ppv)
{
  StdGITEntry* entry;
  HRESULT hres;
  IStream *stream;

  TRACE("dwCookie=0x%x, riid=%s, ppv=%p\n", dwCookie, debugstr_guid(riid), ppv);

  EnterCriticalSection(&git_section);

  entry = StdGlobalInterfaceTable_FindEntry(iface, dwCookie);
  if (entry == NULL) {
    WARN("Entry for cookie 0x%x not found\n", dwCookie);
    LeaveCriticalSection(&git_section);
    return E_INVALIDARG;
  }

  TRACE("entry=%p\n", entry);

  hres = IStream_Clone(entry->stream, &stream);

  LeaveCriticalSection(&git_section);

  if (hres) {
    WARN("Failed to clone stream with error 0x%08x\n", hres);
    return hres;
  }

  /* unmarshal the interface */
  hres = CoUnmarshalInterface(stream, riid, ppv);
  IStream_Release(stream);

  if (hres) {
    WARN("Failed to unmarshal stream\n");
    return hres;
  }

  TRACE("ppv=%p\n", *ppv);
  return S_OK;
}

/* Classfactory definition - despite what MSDN says, some programs need this */

static HRESULT WINAPI
GITCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid, LPVOID *ppv)
{
  *ppv = NULL;
  if (IsEqualIID(riid,&IID_IUnknown) ||
      IsEqualIID(riid,&IID_IGlobalInterfaceTable))
  {
    *ppv = iface;
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG WINAPI GITCF_AddRef(LPCLASSFACTORY iface)
{
  return 2;
}

static ULONG WINAPI GITCF_Release(LPCLASSFACTORY iface)
{
  return 1;
}

static HRESULT WINAPI
GITCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pUnk,
                     REFIID riid, LPVOID *ppv)
{
  if (IsEqualIID(riid,&IID_IGlobalInterfaceTable)) {
    if (StdGlobalInterfaceTableInstance == NULL) 
      StdGlobalInterfaceTableInstance = StdGlobalInterfaceTable_Construct();
    return IGlobalInterfaceTable_QueryInterface( (IGlobalInterfaceTable*) StdGlobalInterfaceTableInstance, riid, ppv);
  }

  FIXME("(%s), not supported.\n",debugstr_guid(riid));
  return E_NOINTERFACE;
}

static HRESULT WINAPI GITCF_LockServer(LPCLASSFACTORY iface, BOOL fLock)
{
    FIXME("(%d), stub!\n",fLock);
    return S_OK;
}

static const IClassFactoryVtbl GITClassFactoryVtbl = {
    GITCF_QueryInterface,
    GITCF_AddRef,
    GITCF_Release,
    GITCF_CreateInstance,
    GITCF_LockServer
};

static const IClassFactoryVtbl *PGITClassFactoryVtbl = &GITClassFactoryVtbl;

HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv)
{
  *ppv = &PGITClassFactoryVtbl;
  TRACE("Returning GIT classfactory\n");
  return S_OK;
}

/* Virtual function table */
static const IGlobalInterfaceTableVtbl StdGlobalInterfaceTableImpl_Vtbl =
{
  StdGlobalInterfaceTable_QueryInterface,
  StdGlobalInterfaceTable_AddRef,
  StdGlobalInterfaceTable_Release,
  StdGlobalInterfaceTable_RegisterInterfaceInGlobal,
  StdGlobalInterfaceTable_RevokeInterfaceFromGlobal,
  StdGlobalInterfaceTable_GetInterfaceFromGlobal
};

/** This function constructs the GIT. It should only be called once **/
void* StdGlobalInterfaceTable_Construct(void)
{
  StdGlobalInterfaceTableImpl* newGIT;

  newGIT = HeapAlloc(GetProcessHeap(), 0, sizeof(StdGlobalInterfaceTableImpl));
  if (newGIT == 0) return newGIT;

  newGIT->lpVtbl = &StdGlobalInterfaceTableImpl_Vtbl;
  newGIT->ref = 1;      /* Initialise the reference count */
  list_init(&newGIT->list);
  newGIT->nextCookie = 0xf100; /* that's where windows starts, so that's where we start */
  TRACE("Created the GIT at %p\n", newGIT);

  return (void*)newGIT;
}
