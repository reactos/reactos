/*
 *	(Local) RPC Stuff
 *
 *  Copyright 2002  Marcus Meissner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"
#include "rpc.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "wtypes.h"
#include "wine/unicode.h"
#include "wine/winbase16.h"
#include "compobj_private.h"
#include "ifs.h"

#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct _wine_rpc_request {
    int				state;
    HANDLE			hPipe;	/* temp copy of handle */
    wine_rpc_request_header	reqh;
    wine_rpc_response_header	resph;
    LPBYTE			Buffer;
} wine_rpc_request;

static wine_rpc_request **reqs = NULL;
static int nrofreqs = 0;

/* This pipe is _thread_ based */
typedef struct _wine_pipe {
    wine_marshal_id	mid;	/* target mid */
    DWORD		tid;	/* thread in which we execute */
    HANDLE		hPipe;

    int			pending;
    HANDLE		hThread;
    CRITICAL_SECTION	crit;
} wine_pipe;

static wine_pipe *pipes = NULL;
static int nrofpipes = 0;

typedef struct _PipeBuf {
    ICOM_VTABLE(IRpcChannelBuffer)	*lpVtbl;
    DWORD				ref;

    wine_marshal_id			mid;
    wine_pipe				*pipe;
} PipeBuf;

static HRESULT WINAPI
_xread(HANDLE hf, LPVOID ptr, DWORD size) {
    DWORD res;
    if (!ReadFile(hf,ptr,size,&res,NULL)) {
	FIXME("Failed to read from %p, le is %lx\n",hf,GetLastError());
	return E_FAIL;
    }
    if (res!=size) {
	FIXME("Read only %ld of %ld bytes from %p.\n",res,size,hf);
	return E_FAIL;
    }
    return S_OK;
}

static void
drs(LPCSTR where) {
#if 0
    static int nrofreaders = 0;

    int i, states[10];

    memset(states,0,sizeof(states));
    for (i=nrofreqs;i--;)
	states[reqs[i]->state]++;
    FIXME("%lx/%s/%d: rq %d, w %d, rg %d, rsq %d, rsg %d, d %d\n",
	    GetCurrentProcessId(),
	    where,
	    nrofreaders,
	    states[REQSTATE_REQ_QUEUED],
	    states[REQSTATE_REQ_WAITING_FOR_REPLY],
	    states[REQSTATE_REQ_GOT],
	    states[REQSTATE_RESP_QUEUED],
	    states[REQSTATE_RESP_GOT],
	    states[REQSTATE_DONE]
    );
#endif

    return ;
}

static HRESULT WINAPI
_xwrite(HANDLE hf, LPVOID ptr, DWORD size) {
    DWORD res;
    if (!WriteFile(hf,ptr,size,&res,NULL)) {
	FIXME("Failed to write to %p, le is %lx\n",hf,GetLastError());
	return E_FAIL;
    }
    if (res!=size) {
	FIXME("Wrote only %ld of %ld bytes to %p.\n",res,size,hf);
	return E_FAIL;
    }
    return S_OK;
}

static DWORD WINAPI _StubReaderThread(LPVOID);

static HRESULT
PIPE_RegisterPipe(wine_marshal_id *mid, HANDLE hPipe, BOOL startreader) {
  int	i;
  char	pipefn[100];
  wine_pipe *new_pipes;

  for (i=0;i<nrofpipes;i++)
    if (pipes[i].mid.processid==mid->processid)
      return S_OK;
  if (pipes)
    new_pipes=(wine_pipe*)HeapReAlloc(GetProcessHeap(),0,pipes,sizeof(pipes[0])*(nrofpipes+1));
  else
    new_pipes=(wine_pipe*)HeapAlloc(GetProcessHeap(),0,sizeof(pipes[0]));
  if (!new_pipes) return E_OUTOFMEMORY;
  pipes = new_pipes;
  sprintf(pipefn,OLESTUBMGR"_%08lx",mid->processid);
  memcpy(&(pipes[nrofpipes].mid),mid,sizeof(*mid));
  pipes[nrofpipes].hPipe	= hPipe;
  InitializeCriticalSection(&(pipes[nrofpipes].crit));
  nrofpipes++;
  if (startreader) {
      pipes[nrofpipes-1].hThread = CreateThread(NULL,0,_StubReaderThread,(LPVOID)(pipes+(nrofpipes-1)),0,&(pipes[nrofpipes-1].tid));
  } else {
      pipes[nrofpipes-1].tid	 = GetCurrentThreadId();
  }
  return S_OK;
}

static HANDLE
PIPE_FindByMID(wine_marshal_id *mid) {
  int i;
  for (i=0;i<nrofpipes;i++)
    if ((pipes[i].mid.processid==mid->processid) &&
	(GetCurrentThreadId()==pipes[i].tid)
    )
      return pipes[i].hPipe;
  return INVALID_HANDLE_VALUE;
}

static wine_pipe*
PIPE_GetFromMID(wine_marshal_id *mid) {
  int i;
  for (i=0;i<nrofpipes;i++) {
    if ((pipes[i].mid.processid==mid->processid) &&
	(GetCurrentThreadId()==pipes[i].tid)
    )
      return pipes+i;
  }
  return NULL;
}

static HRESULT
RPC_GetRequest(wine_rpc_request **req) {
    static int reqid = 0xdeadbeef;
    int i;

    for (i=0;i<nrofreqs;i++) { /* try to reuse */
	if (reqs[i]->state == REQSTATE_DONE) {
	    reqs[i]->reqh.reqid = reqid++;
	    reqs[i]->resph.reqid = reqs[i]->reqh.reqid;
	    reqs[i]->hPipe = INVALID_HANDLE_VALUE;
	    *req = reqs[i];
	    reqs[i]->state = REQSTATE_START;
	    return S_OK;
	}
    }
    /* create new */
    if (reqs)
	reqs = (wine_rpc_request**)HeapReAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			reqs,
			sizeof(wine_rpc_request*)*(nrofreqs+1)
		);
    else
	reqs = (wine_rpc_request**)HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(wine_rpc_request*)
		);
    if (!reqs)
	return E_OUTOFMEMORY;
    reqs[nrofreqs] = (wine_rpc_request*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(wine_rpc_request));
    reqs[nrofreqs]->reqh.reqid = reqid++;
    reqs[nrofreqs]->resph.reqid = reqs[nrofreqs]->reqh.reqid;
    reqs[nrofreqs]->hPipe = INVALID_HANDLE_VALUE;
    *req = reqs[nrofreqs];
    reqs[nrofreqs]->state = REQSTATE_START;
    nrofreqs++;
    return S_OK;
}

static void
RPC_FreeRequest(wine_rpc_request *req) {
    req->state = REQSTATE_DONE; /* Just reuse slot. */
    return;
}

static HRESULT WINAPI
PipeBuf_QueryInterface(
    LPRPCCHANNELBUFFER iface,REFIID riid,LPVOID *ppv
) {
    *ppv = NULL;
    if (IsEqualIID(riid,&IID_IRpcChannelBuffer) || IsEqualIID(riid,&IID_IUnknown)) {
	*ppv = (LPVOID)iface;
	IUnknown_AddRef(iface);
	return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI
PipeBuf_AddRef(LPRPCCHANNELBUFFER iface) {
    ICOM_THIS(PipeBuf,iface);
    This->ref++;
    return This->ref;
}

static ULONG WINAPI
PipeBuf_Release(LPRPCCHANNELBUFFER iface) {
    ICOM_THIS(PipeBuf,iface);
    This->ref--;
    if (This->ref)
	return This->ref;
    ERR("Free all stuff.\n");
    HeapFree(GetProcessHeap(),0,This);
    return 0;
}

static HRESULT WINAPI
PipeBuf_GetBuffer(
    LPRPCCHANNELBUFFER iface,RPCOLEMESSAGE* msg,REFIID riid
) {
    /*ICOM_THIS(PipeBuf,iface);*/

    TRACE("(%p,%s), slightly wrong.\n",msg,debugstr_guid(riid));
    /* probably reuses IID in real. */
    if (msg->cbBuffer && (msg->Buffer == NULL))
	msg->Buffer = HeapAlloc(GetProcessHeap(),0,msg->cbBuffer);
    return S_OK;
}

static HRESULT
_invoke_onereq(wine_rpc_request *req) {
    IRpcStubBuffer	*stub;
    RPCOLEMESSAGE	msg;
    HRESULT		hres;
    DWORD		reqtype;

    hres = MARSHAL_Find_Stub_Buffer(&(req->reqh.mid),&stub);
    if (hres) {
	ERR("Stub not found?\n");
	return hres;
    }
    msg.Buffer		= req->Buffer;
    msg.iMethod		= req->reqh.iMethod;
    msg.cbBuffer	= req->reqh.cbBuffer;
    req->state		= REQSTATE_INVOKING;
    req->resph.retval	= IRpcStubBuffer_Invoke(stub,&msg,NULL);
    req->Buffer		= msg.Buffer;
    req->resph.cbBuffer	= msg.cbBuffer;
    reqtype 		= REQTYPE_RESPONSE;
    hres = _xwrite(req->hPipe,&reqtype,sizeof(reqtype));
    if (hres) return hres;
    hres = _xwrite(req->hPipe,&(req->resph),sizeof(req->resph));
    if (hres) return hres;
    hres = _xwrite(req->hPipe,req->Buffer,req->resph.cbBuffer);
    if (hres) return hres;
    req->state = REQSTATE_DONE;
    drs("invoke");
    return S_OK;
}

static HRESULT _read_one(wine_pipe *xpipe);

static HRESULT
RPC_QueueRequestAndWait(wine_rpc_request *req) {
    int			i;
    wine_rpc_request	*xreq;
    HRESULT		hres;
    DWORD		reqtype;
    wine_pipe		*xpipe = PIPE_GetFromMID(&(req->reqh.mid));

    if (!xpipe) {
	FIXME("no pipe found.\n");
	return E_POINTER;
    }
    if (GetCurrentProcessId() == req->reqh.mid.processid) {
	ERR("In current process?\n");
	return E_FAIL;
    }
    req->hPipe = xpipe->hPipe;
    req->state = REQSTATE_REQ_WAITING_FOR_REPLY;
    reqtype = REQTYPE_REQUEST;
    hres = _xwrite(req->hPipe,&reqtype,sizeof(reqtype));
    if (hres) return hres;
    hres = _xwrite(req->hPipe,&(req->reqh),sizeof(req->reqh));
    if (hres) return hres;
    hres = _xwrite(req->hPipe,req->Buffer,req->reqh.cbBuffer);
    if (hres) return hres;

    while (1) {
	/*WaitForSingleObject(hRpcChanged,INFINITE);*/
	hres = _read_one(xpipe);
	if (hres) break;

	for (i=0;i<nrofreqs;i++) {
	    xreq = reqs[i];
	    if ((xreq->state==REQSTATE_REQ_GOT) && (xreq->hPipe==req->hPipe)) {
		_invoke_onereq(xreq);
	    }
	}
	if (req->state == REQSTATE_RESP_GOT)
	    return S_OK;
    }
    return hres;
}

static HRESULT WINAPI
PipeBuf_SendReceive(
    LPRPCCHANNELBUFFER iface,RPCOLEMESSAGE* msg,ULONG *status
) {
    ICOM_THIS(PipeBuf,iface);
    wine_rpc_request	*req;
    HRESULT		hres;

    TRACE("()\n");

    if (This->mid.processid == GetCurrentProcessId()) {
	ERR("Need to call directly!\n");
	return E_FAIL;
    }

    hres = RPC_GetRequest(&req);
    if (hres) return hres;
    req->reqh.iMethod	= msg->iMethod;
    req->reqh.cbBuffer	= msg->cbBuffer;
    memcpy(&(req->reqh.mid),&(This->mid),sizeof(This->mid));
    req->Buffer = msg->Buffer;
    hres = RPC_QueueRequestAndWait(req);
    if (hres) {
	RPC_FreeRequest(req);
	return hres;
    }
    msg->cbBuffer	= req->resph.cbBuffer;
    msg->Buffer		= req->Buffer;
    *status 		= req->resph.retval;
    RPC_FreeRequest(req);
    return S_OK;
}


static HRESULT WINAPI
PipeBuf_FreeBuffer(LPRPCCHANNELBUFFER iface,RPCOLEMESSAGE* msg) {
    FIXME("(%p), stub!\n",msg);
    return E_FAIL;
}

static HRESULT WINAPI
PipeBuf_GetDestCtx(
    LPRPCCHANNELBUFFER iface,DWORD* pdwDestContext,void** ppvDestContext
) {
    FIXME("(%p,%p), stub!\n",pdwDestContext,ppvDestContext);
    return E_FAIL;
}

static HRESULT WINAPI
PipeBuf_IsConnected(LPRPCCHANNELBUFFER iface) {
    FIXME("(), stub!\n");
    return S_OK;
}

static ICOM_VTABLE(IRpcChannelBuffer) pipebufvt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    PipeBuf_QueryInterface,
    PipeBuf_AddRef,
    PipeBuf_Release,
    PipeBuf_GetBuffer,
    PipeBuf_SendReceive,
    PipeBuf_FreeBuffer,
    PipeBuf_GetDestCtx,
    PipeBuf_IsConnected
};

HRESULT
PIPE_GetNewPipeBuf(wine_marshal_id *mid, IRpcChannelBuffer **pipebuf) {
  wine_marshal_id	ourid;
  DWORD			res;
  HANDLE		hPipe;
  HRESULT		hres;
  PipeBuf		*pbuf;

  hPipe = PIPE_FindByMID(mid);
  if (hPipe == INVALID_HANDLE_VALUE) {
      char			pipefn[200];
      sprintf(pipefn,OLESTUBMGR"_%08lx",mid->processid);
      hPipe = CreateFileA(
	      pipefn,
	      GENERIC_READ|GENERIC_WRITE,
	      0,
	      NULL,
	      OPEN_EXISTING,
	      0,
	      0
      );
      if (hPipe == INVALID_HANDLE_VALUE) {
	  FIXME("Could not open named pipe %s, le is %lx\n",pipefn,GetLastError());
	  return E_FAIL;
      }
      hres = PIPE_RegisterPipe(mid, hPipe, FALSE);
      if (hres) return hres;
      memset(&ourid,0,sizeof(ourid));
      ourid.processid = GetCurrentProcessId();
      if (!WriteFile(hPipe,&ourid,sizeof(ourid),&res,NULL)||(res!=sizeof(ourid))) {
	  ERR("Failed writing startup mid!\n");
	  return E_FAIL;
      }
  }
  pbuf = (PipeBuf*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(PipeBuf));
  pbuf->lpVtbl	= &pipebufvt;
  pbuf->ref 	= 1;
  memcpy(&(pbuf->mid),mid,sizeof(*mid));
  *pipebuf = (IRpcChannelBuffer*)pbuf;
  return S_OK;
}

static HRESULT
create_server(REFCLSID rclsid) {
  HKEY		key;
  char 		buf[200];
  HRESULT	hres = E_UNEXPECTED;
  char		xclsid[80];
  WCHAR 	dllName[MAX_PATH+1];
  DWORD 	dllNameLen = sizeof(dllName);
  STARTUPINFOW	sinfo;
  PROCESS_INFORMATION	pinfo;

  WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);

  sprintf(buf,"CLSID\\%s\\LocalServer32",xclsid);
  hres = RegOpenKeyExA(HKEY_CLASSES_ROOT, buf, 0, KEY_READ, &key);

  if (hres != ERROR_SUCCESS)
      return REGDB_E_READREGDB; /* Probably */

  memset(dllName,0,sizeof(dllName));
  hres= RegQueryValueExW(key,NULL,NULL,NULL,(LPBYTE)dllName,&dllNameLen);
  RegCloseKey(key);
  if (hres)
	  return REGDB_E_CLASSNOTREG; /* FIXME: check retval */
  memset(&sinfo,0,sizeof(sinfo));
  sinfo.cb = sizeof(sinfo);
  if (!CreateProcessW(NULL,dllName,NULL,NULL,FALSE,0,NULL,NULL,&sinfo,&pinfo))
      return E_FAIL;
  return S_OK;
}
/* http://msdn.microsoft.com/library/en-us/dnmsj99/html/com0199.asp, Figure 4 */
HRESULT create_marshalled_proxy(REFCLSID rclsid, REFIID iid, LPVOID *ppv) {
  HRESULT	hres;
  HANDLE	hPipe;
  char		pipefn[200];
  DWORD		res,bufferlen;
  char		marshalbuffer[200];
  IStream	*pStm;
  LARGE_INTEGER	seekto;
  ULARGE_INTEGER newpos;
  int		tries = 0;
#define MAXTRIES 10000

  strcpy(pipefn,PIPEPREF);
  WINE_StringFromCLSID(rclsid,pipefn+strlen(PIPEPREF));

  while (tries++<MAXTRIES) {
      WaitNamedPipeA( pipefn, NMPWAIT_WAIT_FOREVER );
      hPipe	= CreateFileA(
	      pipefn,
	      GENERIC_READ|GENERIC_WRITE,
	      0,
	      NULL,
	      OPEN_EXISTING,
	      0,
	      0
      );
      if (hPipe == INVALID_HANDLE_VALUE) {
	  if (tries == 1) {
	      if ((hres = create_server(rclsid)))
		  return hres;
	      Sleep(1000);
	  } else {
	      WARN("Could not open named pipe to broker %s, le is %lx\n",pipefn,GetLastError());
	      Sleep(1000);
	  }
	  continue;
      }
      bufferlen = 0;
      if (!ReadFile(hPipe,marshalbuffer,sizeof(marshalbuffer),&bufferlen,NULL)) {
	  FIXME("Failed to read marshal id from classfactory of %s.\n",debugstr_guid(rclsid));
	  Sleep(1000);
	  continue;
      }
      CloseHandle(hPipe);
      break;
  }
  if (tries>=MAXTRIES)
      return E_NOINTERFACE;
  hres = CreateStreamOnHGlobal(0,TRUE,&pStm);
  if (hres) return hres;
  hres = IStream_Write(pStm,marshalbuffer,bufferlen,&res);
  if (hres) goto out;
  seekto.u.LowPart = 0;seekto.u.HighPart = 0;
  hres = IStream_Seek(pStm,seekto,SEEK_SET,&newpos);
  hres = CoUnmarshalInterface(pStm,&IID_IClassFactory,ppv);
out:
  IStream_Release(pStm);
  return hres;
}


static void WINAPI
PIPE_StartRequestThread(HANDLE xhPipe) {
    wine_marshal_id	remoteid;
    HRESULT		hres;

    hres = _xread(xhPipe,&remoteid,sizeof(remoteid));
    if (hres) {
	ERR("Failed to read remote mid!\n");
	return;
    }
    PIPE_RegisterPipe(&remoteid,xhPipe, TRUE);
}

static HRESULT
_read_one(wine_pipe *xpipe) {
    DWORD	reqtype;
    HRESULT	hres = S_OK;
    HANDLE	xhPipe = xpipe->hPipe;

    /*FIXME("%lx %d reading reqtype\n",GetCurrentProcessId(),xhPipe);*/
    hres = _xread(xhPipe,&reqtype,sizeof(reqtype));
    if (hres) goto end;
    EnterCriticalSection(&(xpipe->crit));
    /*FIXME("%lx got reqtype %ld\n",GetCurrentProcessId(),reqtype);*/

    if (reqtype == REQTYPE_REQUEST) {
	wine_rpc_request	*xreq;
	RPC_GetRequest(&xreq);
	xreq->hPipe = xhPipe;
	hres = _xread(xhPipe,&(xreq->reqh),sizeof(xreq->reqh));
	if (hres) goto end;
	xreq->resph.reqid = xreq->reqh.reqid;
	xreq->Buffer = HeapAlloc(GetProcessHeap(),0, xreq->reqh.cbBuffer);
	hres = _xread(xhPipe,xreq->Buffer,xreq->reqh.cbBuffer);
	if (hres) goto end;
	xreq->state = REQSTATE_REQ_GOT;
	goto end;
    }
    if (reqtype == REQTYPE_RESPONSE) {
	wine_rpc_response_header	resph;
	int i;

	hres = _xread(xhPipe,&resph,sizeof(resph));
	if (hres) goto end;
	for (i=nrofreqs;i--;) {
	    wine_rpc_request *xreq = reqs[i];
	    if (xreq->state != REQSTATE_REQ_WAITING_FOR_REPLY)
		continue;
	    if (xreq->reqh.reqid == resph.reqid) {
		memcpy(&(xreq->resph),&resph,sizeof(resph));

		if (xreq->Buffer)
		    xreq->Buffer = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,xreq->Buffer,xreq->resph.cbBuffer);
		else
		    xreq->Buffer = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,xreq->resph.cbBuffer);

		hres = _xread(xhPipe,xreq->Buffer,xreq->resph.cbBuffer);
		if (hres) goto end;
		xreq->state = REQSTATE_RESP_GOT;
		/*PulseEvent(hRpcChanged);*/
		goto end;
	    }
	}
	ERR("Did not find request for id %lx\n",resph.reqid);
	hres = S_OK;
	goto end;
    }
    ERR("Unknown reqtype %ld\n",reqtype);
    hres = E_FAIL;
end:
    LeaveCriticalSection(&(xpipe->crit));
    return hres;
}

static DWORD WINAPI
_StubReaderThread(LPVOID param) {
    wine_pipe		*xpipe = (wine_pipe*)param;
    HANDLE		xhPipe = xpipe->hPipe;
    HRESULT		hres;

    TRACE("STUB reader thread %lx\n",GetCurrentProcessId());
    while (1) {
	int i;
	hres = _read_one(xpipe);
	if (hres) break;

	for (i=nrofreqs;i--;) {
	    wine_rpc_request *xreq = reqs[i];
	    if ((xreq->state == REQSTATE_REQ_GOT) && (xreq->hPipe == xhPipe)) {
		_invoke_onereq(xreq);
	    }
	}
    }
    FIXME("Failed with hres %lx\n",hres);
    CloseHandle(xhPipe);
    return 0;
}

static DWORD WINAPI
_StubMgrThread(LPVOID param) {
    char		pipefn[200];
    HANDLE		listenPipe;

    sprintf(pipefn,OLESTUBMGR"_%08lx",GetCurrentProcessId());
    TRACE("Stub Manager Thread starting on (%s)\n",pipefn);

    while (1) {
	listenPipe = CreateNamedPipeA(
	    pipefn,
	    PIPE_ACCESS_DUPLEX,
	    PIPE_TYPE_BYTE|PIPE_WAIT,
	    PIPE_UNLIMITED_INSTANCES,
	    4096,
	    4096,
	    NMPWAIT_USE_DEFAULT_WAIT,
	    NULL
	);
	if (listenPipe == INVALID_HANDLE_VALUE) {
	    FIXME("pipe creation failed for %s, le is %lx\n",pipefn,GetLastError());
	    return 1; /* permanent failure, so quit stubmgr thread */
	}
	if (!ConnectNamedPipe(listenPipe,NULL)) {
	    ERR("Failure during ConnectNamedPipe %lx!\n",GetLastError());
	    CloseHandle(listenPipe);
	    continue;
	}
	PIPE_StartRequestThread(listenPipe);
    }
    return 0;
}

void
STUBMGR_Start() {
  static BOOL stubMgrRunning = FALSE;
  DWORD tid;

  if (!stubMgrRunning) {
      stubMgrRunning = TRUE;
      CreateThread(NULL,0,_StubMgrThread,NULL,0,&tid);
      Sleep(2000); /* actually we just try opening the pipe until it succeeds */
  }
}
