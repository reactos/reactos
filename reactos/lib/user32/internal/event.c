/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include <windows.h>

/***********************************************************************
 * 		EVENT_WaitNetEvent
 *
 * Wait for a network event, optionally sleeping until one arrives.
 * Return TRUE if an event is pending, FALSE on timeout or error
 * (for instance lost connection with the server).
 */
WINBOOL EVENT_WaitNetEvent(WINBOOL sleep, WINBOOL peek)
{
  return TRUE;
}



/**********************************************************************
 *		EVENT_CheckFocus
 */
WINBOOL EVENT_CheckFocus(void)
{
  return TRUE;
}



/**********************************************************************
 *		X11DRV_EVENT_Pending
 */
WINBOOL EVENT_Pending()
{
  return FALSE;
}

UINT EVENT_GetCaptureInfo(void)
{
}

