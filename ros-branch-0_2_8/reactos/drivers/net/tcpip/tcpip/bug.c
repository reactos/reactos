/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/bug.c
 * PURPOSE:     Bugcheck
 * PROGRAMMERS: Art Yerkes
 * REVISIONS:
 */
#include "precomp.h"

VOID TcpipBugCheck( ULONG BugCode ) { KeBugCheck( BugCode ); }
