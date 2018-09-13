/*	File: \foo.c (Created: 01-Nov-1991)
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:16p $
 */

#include <windows.h>
#pragma hdrstop

// #define     DEBUGSTR
#define	BYTE	unsigned char

#include <tdll\stdtyp.h>
#include <tdll\com.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include "foo.h"

#include "xfr_todo.h"

int fooComSendClear(HCOM h, stFB *pB)
	{
    int     rc;

	pB->usSend = 0;
	rc = ComSendClear(h);
    //assert(rc == 0);

    return rc;
	}

#if !defined(FOO_MACRO)

int fooComSendChar(HCOM h, stFB *pB, BYTE c)
	{
    int     rc = 0;

	pB->acSend[pB->usSend++] = c;
	if (pB->usSend >= FB_SIZE)
		{
		rc = ComSndBufrSend(h, (void *)pB->acSend, pB->usSend, 200);
	    //assert(rc == 0);
		pB->usSend = 0;
		}

    return rc;
	}

#endif

int fooComSendPush(HCOM h, stFB *pB)
	{
    int     rc = 0;

	if (pB->usSend > 0)
		{
		rc = ComSndBufrSend(h, (void *)pB->acSend, pB->usSend, 200);
	    //assert(rc == 0);
    	pB->usSend = 0;
		}

    return rc;
	}

int fooComSendCharNow(HCOM h, stFB *pB, BYTE c)
	{
    int     rc;

	rc = fooComSendChar(h, pB, c);
    assert(rc == 0);
    if (rc == 0)
        {
	    rc = fooComSendPush(h, pB);
        //assert(rc == 0);
        }

    return rc;
	}
