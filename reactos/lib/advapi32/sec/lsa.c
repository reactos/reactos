/* $Id: lsa.c,v 1.2 1999/07/26 20:46:39 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/lsa.c
 * PURPOSE:         Local security authority functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990322 EA created
 *	19990515 EA stubs
 */
#include <windows.h>
#include <ddk/ntddk.h>


/**********************************************************************
 *	LsaAddAccountRights
 */
INT
STDCALL
LsaAddAccountRights (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaAddPrivilegesToAccount
 */
INT
STDCALL
LsaAddPrivilegesToAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaClearAuditLog
 */
INT
STDCALL
LsaClearAuditLog (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaClose
 */
INT
STDCALL
LsaClose (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaCreateAccount
 */
INT
STDCALL
LsaCreateAccount (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3	
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaCreateSecret
 */
INT
STDCALL
LsaCreateSecret (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaCreateTrustedDomain
 */
INT
STDCALL
LsaCreateTrustedDomain (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaDelete
 */
INT
STDCALL
LsaDelete (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaDeleteTrustedDomain
 */
INT
STDCALL
LsaDeleteTrustedDomain (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaEnumerateAccountRights
 */
INT
STDCALL
LsaEnumerateAccountRights (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaEnumerateAccounts
 */
INT
STDCALL
LsaEnumerateAccounts (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaEnumerateAccountsWithUserRight
 */
INT
STDCALL
LsaEnumerateAccountsWithUserRight (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaEnumeratePrivileges
 */
INT
STDCALL
LsaEnumeratePrivileges (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaEnumeratePrivilegesOfAccount
 */
INT
STDCALL
LsaEnumeratePrivilegesOfAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaEnumerateTrustedDomains
 */
INT
STDCALL
LsaEnumerateTrustedDomains (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaFreeMemory
 */
INT
STDCALL
LsaFreeMemory (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaGetQuotasForAccount
 */
INT
STDCALL
LsaGetQuotasForAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaGetSystemAccessAccount
 */
INT
STDCALL
LsaGetSystemAccessAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaGetUserName
 */
INT
STDCALL
LsaGetUserName (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaICLookupNames
 */
INT
STDCALL
LsaICLookupNames (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaICLookupSids
 */
INT
STDCALL
LsaICLookupSids (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaLookupNames
 */
INT
STDCALL
LsaLookupNames (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaLookupPrivilegeDisplayName
 */
INT
STDCALL
LsaLookupPrivilegeDisplayName (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaLookupPrivilegeName
 */
INT
STDCALL
LsaLookupPrivilegeName (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaLookupPrivilegeValue
 */
INT
STDCALL
LsaLookupPrivilegeValue (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaLookupSids
 */
INT
STDCALL
LsaLookupSids (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaNtStatusToWinError
 */
INT
STDCALL
LsaNtStatusToWinError (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaOpenAccount
 */
INT
STDCALL
LsaOpenAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaOpenPolicy
 */
INT
STDCALL
LsaOpenPolicy (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaOpenSecret
 */
INT
STDCALL
LsaOpenSecret (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaOpenTrustedDomain
 */
INT
STDCALL
LsaOpenTrustedDomain (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaQueryInfoTrustedDomain
 */
INT
STDCALL
LsaQueryInfoTrustedDomain (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaQueryInformationPolicy
 */
INT
STDCALL
LsaQueryInformationPolicy (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaQuerySecret
 */
INT
STDCALL
LsaQuerySecret (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaQuerySecurityObject
 */
INT
STDCALL
LsaQuerySecurityObject (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaQueryTrustedDomainInfo
 */
INT
STDCALL
LsaQueryTrustedDomainInfo (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaRemoveAccountRights
 */
INT
STDCALL
LsaRemoveAccountRights (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaRemovePrivilegesFromAccount
 */
INT
STDCALL
LsaRemovePrivilegesFromAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaRetrievePrivateData
 */
INT
STDCALL
LsaRetrievePrivateData (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetInformationPolicy
 */
INT
STDCALL
LsaSetInformationPolicy (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetInformationTrustedDomain
 */
INT
STDCALL
LsaSetInformationTrustedDomain (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetQuotasForAccount
 */
INT
STDCALL
LsaSetQuotasForAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetSecret
 */
INT
STDCALL
LsaSetSecret (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetSecurityObject
 */
INT
STDCALL
LsaSetSecurityObject (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetSystemAccessAccount
 */
INT
STDCALL
LsaSetSystemAccessAccount (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaSetTrustedDomainInformation
 */
INT
STDCALL
LsaSetTrustedDomainInformation (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 *	LsaStorePrivateData
 */
INT
STDCALL
LsaStorePrivateData (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */

