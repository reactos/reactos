/* $Id: samlib.h,v 1.1 2004/01/23 10:32:26 ekohl Exp $
*/
/*
 * samlib.h
 *
 * Security Account Manager API, native interface
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by Eric Kohl <ekohl@rz-online.de>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __SAMLIB_H_INCLUDED__
#define __SAMLIB_H_INCLUDED__


BOOL STDCALL
SamInitializeSAM (VOID);

BOOL STDCALL
SamGetDomainSid (PSID *Sid);

BOOL STDCALL
SamSetDomainSid (PSID Sid);

BOOL STDCALL
SamCreateUser (PWSTR UserName,
	       PWSTR UserPassword,
	       PSID UserSid);

BOOL STDCALL
SamCheckUserPassword (PWSTR UserName,
		      PWSTR UserPassword);

BOOL STDCALL
SamGetUserSid (PWSTR UserName,
	       PSID *Sid);

#endif /* __SAMLIB_H_INCLUDED__ */

/* EOF */
