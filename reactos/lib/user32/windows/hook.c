/*
 * Windows hook functions
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *                 1996 Andrew Lewycky
 *
 * Based on investigations by Alex Korobka
 */

#include <windows.h>
#include <user32/hook.h>

#define GetThreadQueue(x) NULL

HANDLE HOOK_systemHooks[WH_NB_HOOKS] = { NULL, };

/***********************************************************************
 *           SetWindowsHookA   
 *
 * FIXME: I don't know if this is correct
 */
HHOOK SetWindowsHookExA(int  idHook, HOOKPROC  lpfn, HINSTANCE  hMod, DWORD  dwThreadId )
{
    return HOOK_SetHook( idHook, lpfn, HOOK_WINA, hMod, dwThreadId );

}


/***********************************************************************
 *           SetWindowsHookExW   
 *
 * FIXME: I don't know if this is correct
 */
HHOOK SetWindowsHookExW(int  idHook, HOOKPROC  lpfn, HINSTANCE  hMod, DWORD  dwThreadId )
{
    return HOOK_SetHook( idHook, lpfn, HOOK_WIN32W, hMod, dwThreadId );

}






/***********************************************************************
 *           UnhookWindowsHook   
 */
WINBOOL STDCALL UnhookWindowsHook( INT id, HOOKPROC proc )
{
	return FALSE;
}


/***********************************************************************
 *           UnhookWindowHookEx   (USER.558)
 */
WINBOOL STDCALL UnhookWindowsHookEx( HHOOK hhook )
{
    return HOOK_RemoveHook( hhook );
}



/***********************************************************************
 *           CallNextHookEx   
 *
 * There aren't ANSI and UNICODE versions of this.
 */
LRESULT STDCALL CallNextHookEx( HHOOK hhook, INT code, WPARAM wParam,
                                 LPARAM lParam )
{
    HHOOK next;
    INT fromtype;	/* figure out Ansi/Unicode */
    HOOKDATA *oldhook;

   
    if (!(next = HOOK_GetNextHook(hhook ))) return 0;

    oldhook = (HOOKDATA *)hhook ;
    fromtype = oldhook->flags & HOOK_MAPTYPE;


    return HOOK_CallHook( next, fromtype, code, wParam, lParam );
}



/***********************************************************************
 *           CallMsgFilterA   (USER.15)
 */
/*
 * FIXME: There are ANSI and UNICODE versions of this, plus an unspecified
 * version, plus USER (the bit one) has a CallMsgFilter function.
 */
WINBOOL STDCALL CallMsgFilterA( LPMSG msg, INT code )
{
   
    if (HOOK_CallHooksA( WH_SYSMSGFILTER, code, 0, (LPARAM)msg ))
      return TRUE;
    return HOOK_CallHooksA( WH_MSGFILTER, code, 0, (LPARAM)msg );
}


/***********************************************************************
 *           CallMsgFilterW   (USER.)
 */
WINBOOL STDCALL CallMsgFilterW( LPMSG msg, INT code )
{
    if (HOOK_CallHooksW( WH_SYSMSGFILTER, code, 0, (LPARAM)msg ))
      return TRUE;
    return HOOK_CallHooksW( WH_MSGFILTER, code, 0, (LPARAM)msg );
}





/***********************************************************************
 *           Internal Functions
 */

/***********************************************************************
 *           HOOK_GetNextHook
 *
 * Get the next hook of a given hook.
 */
HANDLE HOOK_GetNextHook( HHOOK hhook )
{
   
    HOOKDATA *hook;
    if (!hhook) return 0;
    hook = (HOOKDATA *)hhook;

    if (hook->next) return hook->next;
    if (!hook->ownerQueue) return 0;  /* Already system hook */

    /* Now start enumerating the system hooks */
    return HOOK_systemHooks[hook->id - WH_MINHOOK];
}


/***********************************************************************
 *           HOOK_GetHook
 *
 * Get the first hook for a given type.
 */
HANDLE HOOK_GetHook( INT id, HQUEUE hQueue )
{
//    MESSAGEQUEUE *queue;
    HHOOK hook = 0;

 //   if ((queue = (MESSAGEQUEUE *)GlobalLock( hQueue )) != NULL)
 //       hook = queue->hooks[id - WH_MINHOOK];
    if (!hook) hook = HOOK_systemHooks[id - WH_MINHOOK];
    return hook;
}


/***********************************************************************
 *           HOOK_SetHook
 *
 * Install a given hook.
 */
HANDLE HOOK_SetHook( INT id, LPVOID proc, INT type,
			      HINSTANCE hInst, DWORD dwThreadId )
{
    HOOKDATA *data;
    //HANDLE handle;
    HQUEUE hQueue = 0;

    if ((id < WH_MINHOOK) || (id > WH_MAXHOOK)) return 0;

   


    //if (id == WH_JOURNALPLAYBACK) EnableHardwareInput(FALSE);

#if 0
     if (hTask)  /* Task-specific hook */
    {
	if ((id == WH_JOURNALRECORD) || (id == WH_JOURNALPLAYBACK) ||
	    (id == WH_SYSMSGFILTER)) return 0;  /* System-only hooks */
        if (!(hQueue = GetTaskQueue( hTask )))
        {
            /* FIXME: shouldn't this be done somewhere else? */
            if (hTask != GetCurrentTask()) return 0;
            if (!(hQueue = GetFastQueue())) return 0;
        }
    }
#endif

    /* Create the hook structure */

    if (!(data = HeapAlloc(GetProcessHeap(),0, sizeof(HOOKDATA) ))) return 0;
    data->proc        = proc;
    data->id          = id;
    data->ownerQueue  = hQueue;
    data->ownerModule = hInst;
    data->flags       = type;

    /* Insert it in the correct linked list */
#if 0
    if (hQueue)
    {
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
        data->next = queue->hooks[id - WH_MINHOOK];
        queue->hooks[id - WH_MINHOOK] = data;
	return data;
    }
#endif
  
    data->next = HOOK_systemHooks[id - WH_MINHOOK];
    HOOK_systemHooks[id - WH_MINHOOK] = data;
    
  
    return data;
}

/***********************************************************************
 *           HOOK_RemoveHook
 *
 * Remove a hook from the list.
 */
WINBOOL HOOK_RemoveHook( HANDLE hook )
{
    HOOKDATA *data;
    HANDLE *prevHook;



    if (!(data = (HOOKDATA *)(hook))) return FALSE;
    if (data->flags & HOOK_INUSE)
    {
        /* Mark it for deletion later on */
        //WARN(hook, "Hook still running, deletion delayed\n" );
        data->proc = (HOOKPROC)0;
        return TRUE;
    }

//    if (data->id == WH_JOURNALPLAYBACK) EnableHardwareInput(TRUE);
     
    /* Remove it from the linked list */

    if (data->ownerQueue)
    {
#if 0
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock( data->ownerQueue );
        if (!queue) return FALSE;
        prevHook = &queue->hooks[data->id - WH_MINHOOK];
#endif
    }
    else
    	prevHook = &HOOK_systemHooks[data->id - WH_MINHOOK];

    while (*prevHook && *prevHook != hook)
        prevHook = &((HOOKDATA *)(*prevHook))->next;

    if (!*prevHook) return FALSE;
    *prevHook = data->next;
    HeapFree(GetProcessHeap(),0, hook );
    return TRUE;
}


/***********************************************************************
 *           HOOK_FindValidHook
 */
static HANDLE HOOK_FindValidHook( HANDLE hook )
{
    HOOKDATA *data;

    for (;;)
    {
	if (!(data = (HOOKDATA *)(hook))) return 0;
	if (data->proc) return hook;
	hook = data->next;
    }
}


/***********************************************************************
 *           HOOK_CallHook
 *
 * Call a hook procedure.
 */
LRESULT HOOK_CallHook( HHOOK hook, INT fromtype, INT code,
                              WPARAM wParam, LPARAM lParam )
{
   // MESSAGEQUEUE *queue;
    HANDLE prevHook;
    HOOKDATA *data = (HOOKDATA *)(hook);
    LRESULT ret;

    WPARAM wParamOrig = wParam;
    LPARAM lParamOrig = lParam;
   // HOOK_MapFunc MapFunc;
   // HOOK_UnMapFunc UnMapFunc;

   // MapFunc = HOOK_MapFuncs[fromtype][data->flags & HOOK_MAPTYPE];
   // UnMapFunc = HOOK_UnMapFuncs[fromtype][data->flags & HOOK_MAPTYPE];

    //if (MapFunc)
    //  MapFunc( data->id, code, &wParam, &lParam );

    /* Now call it */
#if 0
    if (!(queue = (MESSAGEQUEUE *)GlobalLock( GetThreadQueue(0) ))) return 0;
    prevHook = queue->hCurHook;
    queue->hCurHook = hook;
    data->flags |= HOOK_INUSE;
#endif
    //TRACE(hook, "Calling hook %04x: %d %08x %08lx\n",
    //              hook, code, wParam, lParam );

    ret = data->proc(code, wParam, lParam);

    //TRACE(hook, "Ret hook %04x = %08lx\n", hook, ret );

    data->flags &= ~HOOK_INUSE;
    //queue->hCurHook = prevHook;

    //if (UnMapFunc)
    //  UnMapFunc( data->id, code, wParamOrig, lParamOrig, wParam, lParam );

    if (!data->proc) HOOK_RemoveHook( hook );

    return ret;
}

/***********************************************************************
 *           HOOK_IsHooked
 *
 * Replacement for calling HOOK_GetHook from other modules.
 */
WINBOOL HOOK_IsHooked( INT id )
{
    return HOOK_GetHook( id, GetThreadQueue(0) ) != 0;
}



/***********************************************************************
 *           HOOK_CallHooksA
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooksA( INT id, INT code, WPARAM wParam,
                           LPARAM lParam )
{
    HANDLE hook; 

    if (!(hook = HOOK_GetHook( id , GetThreadQueue(0) ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WINA, code, wParam, lParam );
}

/***********************************************************************
 *           HOOK_CallHooksW
 *
 * Call a hook chain.
 */
LRESULT HOOK_CallHooksW( INT id, INT code, WPARAM wParam,
                           LPARAM lParam )
{
    HANDLE hook; 

    if (!(hook = HOOK_GetHook( id , GetThreadQueue(0) ))) return 0;
    if (!(hook = HOOK_FindValidHook(hook))) return 0;
    return HOOK_CallHook( hook, HOOK_WINW, code, wParam,
			  lParam );
}


/***********************************************************************
 *           HOOK_ResetQueueHooks
 */
void HOOK_ResetQueueHooks( HQUEUE hQueue )
{
#if 0
    MESSAGEQUEUE *queue;

    if ((queue = (MESSAGEQUEUE *)GlobalLock( hQueue )) != NULL)
    {
	HOOKDATA*	data;
	HHOOK		hook;
	int		id;
	for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
	{
	    hook = queue->hooks[id - WH_MINHOOK];
	    while( hook )
	    {
	        if( (data = (HOOKDATA *)(hook)) )
	        {
		  data->ownerQueue = hQueue;
		  hook = data->next;
		} else break;
	    }
	}
    }
#endif
}

/***********************************************************************
 *	     HOOK_FreeModuleHooks
 */
void HOOK_FreeModuleHooks( HMODULE hModule )
{
 /* remove all system hooks registered by this module */

  HOOKDATA*     hptr;
  HHOOK         hook, next;
  int           id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       hook = HOOK_systemHooks[id - WH_MINHOOK];
       while( hook )
          if( (hptr = (HOOKDATA *)(hook)) )
	    {
	      next = hptr->next;
	      if( hptr->ownerModule == hModule )
                {
                  hptr->flags &= HOOK_MAPTYPE;
                  HOOK_RemoveHook(hook);
                }
	      hook = next;
	    }
	  else hook = 0;
    }
}

/***********************************************************************
 *	     HOOK_FreeQueueHooks
 */
void HOOK_FreeQueueHooks( HQUEUE hQueue )
{
  /* remove all hooks registered by this queue */

  HOOKDATA*	hptr = NULL;
  HHOOK 	hook, next;
  int 		id;

  for( id = WH_MINHOOK; id <= WH_MAXHOOK; id++ )
    {
       hook = HOOK_GetHook( id, hQueue );
       while( hook )
	{
	  next = HOOK_GetNextHook(hook);

	  hptr = (HOOKDATA *)(hook);
	  if( hptr && hptr->ownerQueue == hQueue )
	    {
	      hptr->flags &= HOOK_MAPTYPE;
	      HOOK_RemoveHook(hook);
	    }
	  hook = next;
	}
    }
}
