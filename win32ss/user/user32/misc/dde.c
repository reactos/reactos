/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS user32.dll
 * PURPOSE:          Dynamic Data Exchange
 * FILE:             win32ss/user/user32/misc/dde.c
 * PROGRAMER:                                                          
 */

#include <user32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(ddeml);


BOOL FASTCALL DdeAddPair(HGLOBAL ClientMem, HGLOBAL ServerMem);
HGLOBAL FASTCALL DdeGetPair(HGLOBAL ServerMem);


/* description of the data fields that need to be packed along with a sent message */
struct packed_message
{
    //union packed_structs ps;
    int         count;
    const void *data;
    int         size;
};

/* add a data field to a packed message */
static inline void push_data( struct packed_message *data, const void *ptr, int size )
{
    data->data = ptr;
    data->size = size;
    data->count++;
}

/* pack a pointer into a 32/64 portable format */
static inline ULONGLONG pack_ptr( const void *ptr )
{
    return (ULONG_PTR)ptr;
}

/* unpack a potentially 64-bit pointer, returning 0 when truncated */
static inline void *unpack_ptr( ULONGLONG ptr64 )
{
    if ((ULONG_PTR)ptr64 != ptr64) return 0;
       return (void *)(ULONG_PTR)ptr64;
}


/***********************************************************************
 *		post_dde_message
 *
 * Post a DDE message
 */
BOOL post_dde_message( struct packed_message *data, UINT message, LPARAM lParam , LPARAM *lp)
{
    void*       ptr = NULL;
    int         size = 0;
    UINT_PTR    uiLo, uiHi;
    HGLOBAL     hunlock = 0;
    ULONGLONG   hpack;

    if (!UnpackDDElParam( message, lParam, &uiLo, &uiHi ))
    {
        ERR("Unpack failed %x\n",message);
        return FALSE;
    }

    *lp = lParam;
    switch (message)
    {
        /* DDE messages which don't require packing are:
         * WM_DDE_INITIATE
         * WM_DDE_TERMINATE
         * WM_DDE_REQUEST
         * WM_DDE_UNADVISE
         */
    case WM_DDE_ACK:
        if (HIWORD(uiHi))
        {
            /* uiHi should contain a hMem from WM_DDE_EXECUTE */
            HGLOBAL h = DdeGetPair( (HANDLE)uiHi );
            if (h)
            {
                hpack = pack_ptr( h );
                /* send back the value of h on the other side */
                push_data( data, &hpack, sizeof(hpack) );
                *lp = uiLo;
                TRACE( "send dde-ack %lx %08lx => %p\n", uiLo, uiHi, h );
            }
        }
        else
        {
            /* uiHi should contain either an atom or 0 */
            TRACE( "send dde-ack %lx atom=%lx\n", uiLo, uiHi );
            *lp = MAKELONG( uiLo, uiHi );
        }
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        size = 0;
        if (uiLo)
        {
            size = GlobalSize( (HGLOBAL)uiLo ) ;
            TRACE("WM_DDE_A D P size %d\n",size);
            if ( (message == WM_DDE_ADVISE && size < sizeof(DDEADVISE)) ||
                 (message == WM_DDE_DATA   && size < FIELD_OFFSET(DDEDATA, Value)) ||
                 (message == WM_DDE_POKE   && size < FIELD_OFFSET(DDEPOKE, Value)) )
            return FALSE;
        }
        else if (message != WM_DDE_DATA)
        {
            TRACE("WM_DDE uiLo 0\n");
            return FALSE;
        }

        *lp = uiHi;
        if (uiLo)
        {
            if ((ptr = GlobalLock( (HGLOBAL)uiLo) ))
            {
                DDEDATA *dde_data = ptr;
                TRACE("unused %d, fResponse %d, fRelease %d, fDeferUpd %d, fAckReq %d, cfFormat %d\n",
                       dde_data->unused, dde_data->fResponse, dde_data->fRelease,
                       dde_data->reserved, dde_data->fAckReq, dde_data->cfFormat);
                push_data( data, ptr, size );
                hunlock = (HGLOBAL)uiLo;
            }
        }
        TRACE( "send ddepack %u %lx\n", size, uiHi );
        break;
    case WM_DDE_EXECUTE:
        if (lParam)
        {
            if ((ptr = GlobalLock( (HGLOBAL)lParam) ))
            {
                size = GlobalSize( (HGLOBAL)lParam );
                push_data(data, ptr, size);
                /* so that the other side can send it back on ACK */
                *lp = lParam;
                hunlock = (HGLOBAL)lParam;
                TRACE("WM_DDE_EXECUTE text size %d\n",GlobalSize( (HGLOBAL)lParam ));
            }
        }
        break;
    }

    FreeDDElParam(message, lParam);

    if (hunlock) GlobalUnlock(hunlock);

    return TRUE;
}

/***********************************************************************
 *		unpack_dde_message
 *
 * Unpack a posted DDE message received from another process.
 */
BOOL unpack_dde_message( HWND hwnd, UINT message, LPARAM *lparam, PVOID buffer, int size )
{
    UINT_PTR	uiLo, uiHi;
    HGLOBAL	hMem = 0;
    void*	ptr;

    TRACE("udm : Size %d\n",size);

    switch (message)
    {
    case WM_DDE_ACK:
        if (size)
        {
            ULONGLONG hpack;
            /* hMem is being passed */
            if (size != sizeof(hpack)) return FALSE;
            if (!buffer) return FALSE;
            uiLo = *lparam;
            memcpy( &hpack, buffer, size );
            hMem = unpack_ptr( hpack );
            uiHi = (UINT_PTR)hMem;
            TRACE("recv dde-ack %lx mem=%lx[%lx]\n", uiLo, uiHi, GlobalSize( hMem ));
        }
        else
        {
            uiLo = LOWORD( *lparam );
            uiHi = HIWORD( *lparam );
            TRACE("recv dde-ack %lx atom=%lx\n", uiLo, uiHi);
        }
	*lparam = PackDDElParam( WM_DDE_ACK, uiLo, uiHi );
	break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
	if ((!buffer) && message != WM_DDE_DATA) return FALSE;
	uiHi = *lparam;
        if (size)
        {
            if (!(hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size )))
                return FALSE;
            if ((ptr = GlobalLock( hMem )))
            {
                memcpy( ptr, buffer, size );
                GlobalUnlock( hMem );
            }
            else
            {
                GlobalFree( hMem );
                return FALSE;
            }
        }
        uiLo = (UINT_PTR)hMem;

	*lparam = PackDDElParam( message, uiLo, uiHi );
	break;
    case WM_DDE_EXECUTE:
	if (size)
	{
	    if (!buffer) return FALSE;
            if (!(hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size ))) return FALSE;
            if ((ptr = GlobalLock( hMem )))
	    {
		memcpy( ptr, buffer, size );
		GlobalUnlock( hMem );
                TRACE( "exec: pairing c=%08lx s=%p\n", *lparam, hMem );
                if (!DdeAddPair( (HGLOBAL)*lparam, hMem ))
                {
                    GlobalFree( hMem );
                    TRACE("udm exec: GF 1\n");
                    return FALSE;
                }
            }
            else
            {
                GlobalFree( hMem );
                TRACE("udm exec: GF 2\n");
                return FALSE;
            }
	}
	else
        {
	    TRACE("udm exec: No Size\n");
	    return FALSE;
        }

        TRACE( "exec: exit c=%08lx s=%p\n", *lparam, hMem );
        *lparam = (LPARAM)hMem;
        break;
    }
    return TRUE;
}

//
//  DDE Post kernel callback.
//
NTSTATUS
WINAPI
User32CallDDEPostFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  struct packed_message data;
  BOOL Ret;
  NTSTATUS Status = STATUS_SUCCESS;
  PDDEPOSTGET_CALLBACK_ARGUMENTS Common = Arguments;

  data.data = 0;
  data.size = 0;
  TRACE("DDE Post CB\n");
  Ret = post_dde_message( &data, Common->message, Common->lParam, &Common->lParam);

  if (Ret)
  {
     Common->pvData = (PVOID)data.data;
     Common->size = data.size;
     TRACE("DDE Post CB size %d\n",data.size);
  }
  else
  {
     ERR("DDE Post CB Return bad msg 0x%x Size %d\n",Common->message,Common->size);
     Status = STATUS_UNSUCCESSFUL;
  }

  return ZwCallbackReturn(Arguments, ArgumentLength, Status);
}

//
//  DDE Get/Peek kernel callback.
//
NTSTATUS
WINAPI
User32CallDDEGetFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  BOOL Ret;
  NTSTATUS Status = STATUS_SUCCESS;
  PDDEPOSTGET_CALLBACK_ARGUMENTS Common = Arguments;

  TRACE("DDE Get CB size %d\n",Common->size);

  Ret = unpack_dde_message( Common->hwnd, Common->message, &Common->lParam, Common->buffer, Common->size );

  if (!Ret)
  {
     ERR("DDE Get CB Return bad msg 0x%x\n",Common->message);
     Status = STATUS_UNSUCCESSFUL;
  }
  return ZwCallbackReturn(Arguments, ArgumentLength, Status);
}


/*
 * @unimplemented
 */
BOOL WINAPI DdeGetQualityOfService(HWND hWnd, DWORD Reserved, PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  UNIMPLEMENTED;
  return FALSE;
}
