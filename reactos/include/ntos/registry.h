/* $Id: registry.h,v 1.1 2001/01/28 21:32:37 ekohl Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/registry.h
 * PURPOSE:      Registry declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   Eric Kohl <ekohl@rz-online.de>
 * UPDATE HISTORY: 
 *               25/01/2001: Created
 */

#ifndef __INCLUDE_REGISTRY_H
#define __INCLUDE_REGISTRY_H

/* Key access rights */
#define KEY_QUERY_VALUE	(1)
#define KEY_SET_VALUE	(2)
#define KEY_CREATE_SUB_KEY	(4)
#define KEY_ENUMERATE_SUB_KEYS	(8)
#define KEY_NOTIFY	(16)
#define KEY_CREATE_LINK	(32)

#define KEY_READ	(0x20019L)
#define KEY_WRITE	(0x20006L)
#define KEY_EXECUTE	(0x20019L)
#define KEY_ALL_ACCESS	(0xf003fL)


/* RegCreateKeyEx */
#define REG_OPTION_VOLATILE	(0x1L)
#define REG_OPTION_NON_VOLATILE	(0L)
#define REG_CREATED_NEW_KEY	(0x1L)
#define REG_OPENED_EXISTING_KEY	(0x2L)


#endif /* __INCLUDE_REGISTRY_H */
