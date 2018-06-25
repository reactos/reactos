@ stub DsAddSidHistoryA
@ stub DsAddSidHistoryW
@ stdcall DsBindA(str str ptr)
@ stdcall DsBindW(wstr wstr ptr)
@ stub DsBindWithCredA
@ stub DsBindWithCredW
@ stub DsBindWithSpnA
@ stub DsBindWithSpnW
@ stub DsClientMakeSpnForTargetServerA
@ stdcall DsClientMakeSpnForTargetServerW(wstr wstr ptr ptr)
@ stdcall DsCrackNamesA(ptr long long long long ptr ptr)
@ stdcall DsCrackNamesW(ptr long long long long ptr ptr)
@ stub DsCrackSpn2A
@ stub DsCrackSpn2W
@ stub DsCrackSpn3W
@ stub DsCrackSpnA
@ stub DsCrackSpnW
@ stub DsCrackUnquotedMangledRdnA
@ stub DsCrackUnquotedMangledRdnW
@ stub DsFreeDomainControllerInfoA
@ stub DsFreeDomainControllerInfoW
@ stub DsFreeNameResultA
@ stub DsFreeNameResultW
@ stub DsFreePasswordCredentials
@ stub DsFreeSchemaGuidMapA
@ stub DsFreeSchemaGuidMapW
@ stub DsFreeSpnArrayA
@ stub DsFreeSpnArrayW
@ stub DsGetDomainControllerInfoA
@ stub DsGetDomainControllerInfoW
@ stub DsGetRdnW
@ stdcall DsGetSpnA(long str str long long ptr ptr ptr ptr)
@ stub DsGetSpnW
@ stub DsInheritSecurityIdentityA
@ stub DsInheritSecurityIdentityW
@ stub DsIsMangledDnA
@ stub DsIsMangledDnW
@ stub DsIsMangledRdnValueA
@ stub DsIsMangledRdnValueW
@ stub DsListDomainsInSiteA
@ stub DsListDomainsInSiteW
@ stub DsListInfoForServerA
@ stub DsListInfoForServerW
@ stub DsListRolesA
@ stub DsListRolesW
@ stub DsListServersForDomainInSiteA
@ stub DsListServersForDomainInSiteW
@ stub DsListServersInSiteA
@ stub DsListServersInSiteW
@ stub DsListSitesA
@ stub DsListSitesW
@ stub DsLogEntry
@ stub DsMakePasswordCredentialsA
@ stub DsMakePasswordCredentialsW
@ stdcall DsMakeSpnA(str str str long str ptr ptr)
@ stdcall DsMakeSpnW(wstr wstr wstr long wstr ptr ptr)
@ stub DsMapSchemaGuidsA
@ stub DsMapSchemaGuidsW
@ stub DsQuoteRdnValueA
@ stub DsQuoteRdnValueW
@ stub DsRemoveDsDomainA
@ stub DsRemoveDsDomainW
@ stub DsRemoveDsServerA
@ stub DsRemoveDsServerW
@ stub DsReplicaAddA
@ stub DsReplicaAddW
@ stub DsReplicaConsistencyCheck
@ stub DsReplicaDelA
@ stub DsReplicaDelW
@ stub DsReplicaFreeInfo
@ stub DsReplicaGetInfo2W
@ stub DsReplicaGetInfoW
@ stub DsReplicaModifyA
@ stub DsReplicaModifyW
@ stub DsReplicaSyncA
@ stub DsReplicaSyncAllA
@ stub DsReplicaSyncAllW
@ stub DsReplicaSyncW
@ stub DsReplicaUpdateRefsA
@ stub DsReplicaUpdateRefsW
@ stub DsReplicaVerifyObjectsA
@ stub DsReplicaVerifyObjectsW
@ stdcall DsServerRegisterSpnA(long str str)
@ stdcall DsServerRegisterSpnW(long wstr wstr)
@ stub DsUnBindA
@ stub DsUnBindW
@ stub DsUnquoteRdnValueA
@ stub DsUnquoteRdnValueW
@ stub DsWriteAccountSpnA
@ stub DsWriteAccountSpnW
@ stub DsaopBind
@ stub DsaopBindWithCred
@ stub DsaopBindWithSpn
@ stub DsaopExecuteScript
@ stub DsaopPrepareScript
@ stub DsaopUnBind
