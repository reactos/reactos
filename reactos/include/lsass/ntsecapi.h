/* $Id: ntsecapi.h,v 1.1 2000/08/12 19:33:18 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            include/lsass/ntsecpai.h
 * PURPOSE:         LSASS API declarations
 * UPDATE HISTORY:
 *                  Created 05/08/00
 */

#ifndef __INCLUDE_LSASS_NTSECAPI_H
#define __INCLUDE_LSASS_NTSECAPI_H

#define SECURITY_LOGON_INTERACTIVE        (2)
#define SECURITY_LOGON_NETWORK            (3)
#define SECURITY_LOGON_BATCH              (4)
#define SECURITY_LOGON_SERVICE            (5)
#define SECURITY_LOGON_PROXY              (6)
#define SECURITY_LOGON_UNLOCK             (7)

typedef ULONG SECURITY_LOGON_TYPE;

typedef struct _LSA_STRING
{
   USHORT Length;
   USHORT MaximumLength;
   PWSTR Buffer;
} LSA_STRING, *PLSA_STRING;

typedef ULONG LSA_OPERATIONAL_MODE, *PLSA_OPERATIONAL_MODE;

#endif /* __INCLUDE_LSASS_NTSECAPI_H */
