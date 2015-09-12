1 stub I_ScGetCurrentGroupStateW
2 stdcall A_SHAFinal(ptr ptr)
3 stdcall A_SHAInit(ptr)
4 stdcall A_SHAUpdate(ptr ptr long)
5 stdcall AbortSystemShutdownA(ptr)
6 stdcall AbortSystemShutdownW(ptr)
7 stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr)
8 stdcall AccessCheckAndAuditAlarmA(str ptr str str ptr long ptr long ptr ptr ptr)
9 stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr)
10 stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr)
11 stdcall AccessCheckByTypeAndAuditAlarmA(str ptr str str ptr ptr long long long ptr long ptr long ptr ptr ptr)
12 stdcall AccessCheckByTypeAndAuditAlarmW(wstr ptr wstr wstr ptr ptr long long long ptr long ptr long ptr ptr ptr)
13 stub AccessCheckByTypeResultList
14 stdcall AccessCheckByTypeResultListAndAuditAlarmA(str ptr str str ptr long long long long ptr long ptr long ptr ptr ptr)
15 stdcall AccessCheckByTypeResultListAndAuditAlarmByHandleA(str ptr ptr str str ptr long long long long ptr long ptr long ptr ptr ptr)
16 stdcall AccessCheckByTypeResultListAndAuditAlarmByHandleW(wstr ptr ptr wstr wstr ptr long long long long ptr long ptr long ptr ptr ptr)
17 stdcall AccessCheckByTypeResultListAndAuditAlarmW(wstr ptr wstr wstr ptr long long long long ptr long ptr long ptr ptr ptr)
18 stdcall AddAccessAllowedAce(ptr long long ptr)
19 stdcall AddAccessAllowedAceEx(ptr long long long ptr)
20 stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
21 stdcall AddAccessDeniedAce(ptr long long ptr)
22 stdcall AddAccessDeniedAceEx(ptr long long long ptr)
23 stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
24 stdcall AddAce(ptr long long ptr long)
25 stdcall AddAuditAccessAce(ptr long long ptr long long)
26 stdcall AddAuditAccessAceEx(ptr long long long ptr long long)
27 stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
28 stdcall AddUsersToEncryptedFile(wstr ptr)
29 stdcall AdjustTokenGroups(long long ptr long ptr ptr)
30 stdcall AdjustTokenPrivileges(long long ptr long ptr ptr)
31 stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr)
32 stdcall AllocateLocallyUniqueId(ptr)
33 stdcall AreAllAccessesGranted(long long)
34 stdcall AreAnyAccessesGranted(long long)
35 stdcall BackupEventLogA(long str)
36 stdcall BackupEventLogW(long wstr)
37 stdcall BuildExplicitAccessWithNameA(ptr str long long long)
38 stdcall BuildExplicitAccessWithNameW(ptr wstr long long long)
39 stdcall BuildImpersonateExplicitAccessWithNameA(ptr str ptr long long long)
40 stdcall BuildImpersonateExplicitAccessWithNameW(ptr wstr ptr long long long)
41 stdcall BuildImpersonateTrusteeA(ptr ptr)
42 stdcall BuildImpersonateTrusteeW(ptr ptr)
43 stdcall BuildSecurityDescriptorA(ptr ptr long ptr long ptr ptr ptr ptr)
44 stdcall BuildSecurityDescriptorW(ptr ptr long ptr long ptr ptr ptr ptr)
45 stdcall BuildTrusteeWithNameA(ptr str)
46 stdcall BuildTrusteeWithNameW(ptr wstr)
47 stdcall BuildTrusteeWithObjectsAndNameA(ptr ptr long str str str)
48 stdcall BuildTrusteeWithObjectsAndNameW(ptr ptr long wstr wstr wstr)
49 stdcall BuildTrusteeWithObjectsAndSidA(ptr ptr ptr ptr ptr)
50 stdcall BuildTrusteeWithObjectsAndSidW(ptr ptr ptr ptr ptr)
51 stdcall BuildTrusteeWithSidA(ptr ptr)
52 stdcall BuildTrusteeWithSidW(ptr ptr)
53 stub CancelOverlappedAccess
54 stdcall ChangeServiceConfig2A(long long ptr)
55 stdcall ChangeServiceConfig2W(long long ptr)
56 stdcall ChangeServiceConfigA(long long long long wstr str ptr str str str str)
57 stdcall ChangeServiceConfigW(long long long long wstr wstr ptr wstr wstr wstr wstr)
58 stdcall CheckTokenMembership(long ptr ptr)
59 stdcall ClearEventLogA(long str)
60 stdcall ClearEventLogW(long wstr)
61 stub CloseCodeAuthzLevel
62 stub CloseEncryptedFileRaw
63 stdcall CloseEventLog(long)
64 stdcall CloseServiceHandle(long)
65 stub CloseTrace
66 stdcall CommandLineFromMsiDescriptor(wstr ptr ptr)
67 stub ComputeAccessTokenFromCodeAuthzLevel
68 stdcall ControlService(long long ptr)
69 stdcall ControlTraceA(double str ptr long) ntdll.EtwControlTraceA
70 stdcall ControlTraceW(double wstr ptr long) ntdll.EtwControlTraceW
71 stub ConvertAccessToSecurityDescriptorA
72 stub ConvertAccessToSecurityDescriptorW
73 stub ConvertSDToStringSDRootDomainA
74 stub ConvertSDToStringSDRootDomainW
75 stub ConvertSecurityDescriptorToAccessA
76 stub ConvertSecurityDescriptorToAccessNamedA #ConvertSecurityDescriptorToAccessA
77 stub ConvertSecurityDescriptorToAccessNamedW #ConvertSecurityDescriptorToAccessW
78 stub ConvertSecurityDescriptorToAccessW
79 stdcall ConvertSecurityDescriptorToStringSecurityDescriptorA(ptr long long ptr ptr)
80 stdcall ConvertSecurityDescriptorToStringSecurityDescriptorW(ptr long long ptr ptr)
81 stdcall ConvertSidToStringSidA(ptr ptr)
82 stdcall ConvertSidToStringSidW(ptr ptr)
83 stub ConvertStringSDToSDDomainA
84 stub ConvertStringSDToSDDomainW
85 stub ConvertStringSDToSDRootDomainA
86 stub ConvertStringSDToSDRootDomainW
87 stdcall ConvertStringSecurityDescriptorToSecurityDescriptorA(str long ptr ptr)
88 stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr)
89 stdcall ConvertStringSidToSidA(ptr ptr)
90 stdcall ConvertStringSidToSidW(ptr ptr)
91 stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr)
92 stdcall CopySid(long ptr ptr)
93 stub CreateCodeAuthzLevel
94 stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr)
95 stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long ptr ptr)
96 stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long ptr ptr)
97 stdcall CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr)
98 stdcall CreateProcessAsUserW(long str str ptr ptr long long ptr str ptr ptr)
99 stdcall CreateProcessWithLogonW(wstr wstr wstr long wstr wstr long ptr wstr ptr ptr)
100 stdcall CreateProcessWithTokenW(ptr long wstr wstr long ptr wstr ptr ptr)
101 stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr)
102 stdcall CreateServiceA(long str str long long long long str str ptr str str str)
103 stdcall CreateServiceW(long wstr wstr long long long long wstr wstr ptr wstr wstr wstr)
104 stdcall CreateTraceInstanceId(ptr ptr) ntdll.EtwCreateTraceInstanceId
105 stdcall CreateWellKnownSid(long ptr ptr ptr)
106 stdcall CredDeleteA(str long long)
107 stdcall CredDeleteW(wstr long long)
108 stdcall CredEnumerateA(str long ptr ptr)
109 stdcall CredEnumerateW(wstr long ptr ptr)
110 stdcall CredFree(ptr)
111 stdcall CredGetSessionTypes(long ptr)
112 stub CredGetTargetInfoA
113 stub CredGetTargetInfoW
114 stdcall CredIsMarshaledCredentialA(str)
115 stdcall CredIsMarshaledCredentialW(wstr)
116 stdcall CredMarshalCredentialA(long ptr str)
117 stdcall CredMarshalCredentialW(long ptr wstr)
118 stub CredProfileLoaded
119 stdcall CredReadA(str long long ptr)
120 stdcall CredReadDomainCredentialsA(ptr long ptr ptr)
121 stdcall CredReadDomainCredentialsW(ptr long ptr ptr)
122 stdcall CredReadW(wstr long long ptr)
123 stub CredRenameA
124 stub CredRenameW
125 stdcall CredUnmarshalCredentialA(str ptr ptr)
126 stdcall CredUnmarshalCredentialW(wstr ptr ptr)
127 stdcall CredWriteA(ptr long)
# CredWriteDomainCredentialsA
# CredWriteDomainCredentialsW
130 stdcall CredWriteW(ptr long)
131 stub CredpConvertCredential
132 stub CredpConvertTargetInfo
133 stub CredpDecodeCredential
134 stub CredpEncodeCredential
135 stdcall CryptAcquireContextA(ptr str str long long)
136 stdcall CryptAcquireContextW(ptr wstr wstr long long)
137 stdcall CryptContextAddRef(long ptr long)
138 stdcall CryptCreateHash(long long long long ptr)
139 stdcall CryptDecrypt(long long long long ptr ptr)
140 stdcall CryptDeriveKey(long long long long ptr)
141 stdcall CryptDestroyHash(long)
142 stdcall CryptDestroyKey(long)
143 stdcall CryptDuplicateHash(long ptr long ptr)
144 stdcall CryptDuplicateKey(long ptr long ptr)
145 stdcall CryptEncrypt(long long long long ptr ptr long)
146 stdcall CryptEnumProviderTypesA(long ptr long ptr ptr ptr)
147 stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr)
148 stdcall CryptEnumProvidersA(long ptr long ptr ptr ptr)
149 stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr)
150 stdcall CryptExportKey(long long long long ptr ptr)
151 stdcall CryptGenKey(long long long ptr)
152 stdcall CryptGenRandom(long long ptr)
153 stdcall CryptGetDefaultProviderA(long ptr long ptr ptr)
154 stdcall CryptGetDefaultProviderW(long ptr long ptr ptr)
155 stdcall CryptGetHashParam(long long ptr ptr long)
156 stdcall CryptGetKeyParam(long long ptr ptr long)
157 stdcall CryptGetProvParam(long long ptr ptr long)
158 stdcall CryptGetUserKey(long long ptr)
159 stdcall CryptHashData(long ptr long long)
160 stdcall CryptHashSessionKey(long long long)
161 stdcall CryptImportKey(long ptr long long long ptr)
162 stdcall CryptReleaseContext(long long)
163 stdcall CryptSetHashParam(long long ptr long)
164 stdcall CryptSetKeyParam(long long ptr long)
165 stdcall CryptSetProvParam(long long ptr long)
166 stdcall CryptSetProviderA(str long)
167 stdcall CryptSetProviderExA(str long ptr long)
168 stdcall CryptSetProviderExW(wstr long ptr long)
169 stdcall CryptSetProviderW(wstr long)
170 stdcall CryptSignHashA(long long ptr long ptr ptr)
171 stdcall CryptSignHashW(long long ptr long ptr ptr)
172 stdcall CryptVerifySignatureA(long ptr long long ptr long)
173 stdcall CryptVerifySignatureW(long ptr long long ptr long)
174 stdcall DecryptFileA(str long)
175 stdcall DecryptFileW(wstr long)
176 stdcall DeleteAce(ptr long)
177 stdcall DeleteService(long)
178 stdcall DeregisterEventSource(long)
179 stdcall DestroyPrivateObjectSecurity(ptr)
180 stub DuplicateEncryptionInfoFile
181 stdcall DuplicateToken(long long ptr)
182 stdcall DuplicateTokenEx(long long ptr long long ptr)
183 stub ElfBackupEventLogFileA
184 stub ElfBackupEventLogFileW
185 stub ElfChangeNotify
186 stub ElfClearEventLogFileA
187 stub ElfClearEventLogFileW
188 stub ElfCloseEventLog
189 stdcall ElfDeregisterEventSource(long)
190 stub ElfFlushEventLog
191 stub ElfNumberOfRecords
192 stub ElfOldestRecord
193 stub ElfOpenBackupEventLogA
194 stub ElfOpenBackupEventLogW
195 stub ElfOpenEventLogA
196 stub ElfOpenEventLogW
197 stub ElfReadEventLogA
198 stub ElfReadEventLogW
199 stub ElfRegisterEventSourceA
200 stdcall ElfRegisterEventSourceW(ptr ptr ptr)
201 stub ElfReportEventA
202 stub ElfReportEventAndSourceW
203 stdcall ElfReportEventW(long long long long ptr long long ptr ptr ptr ptr ptr)
204 stdcall EnableTrace(long long long ptr double) ntdll.EtwEnableTrace
205 stdcall EncryptFileA(str)
206 stdcall EncryptFileW(wstr)
207 stub EncryptedFileKeyInfo
208 stdcall EncryptionDisable(wstr long)
209 stdcall EnumDependentServicesA(long long ptr long ptr ptr)
210 stdcall EnumDependentServicesW(long long ptr long ptr ptr)
211 stdcall EnumServiceGroupW(ptr long long ptr long ptr ptr ptr wstr)
212 stdcall EnumServicesStatusA(long long long ptr long ptr ptr ptr)
213 stdcall EnumServicesStatusExA(long long long long ptr long ptr ptr ptr str)
214 stdcall EnumServicesStatusExW(long long long long ptr long ptr ptr ptr wstr)
215 stdcall EnumServicesStatusW(long long long ptr long ptr ptr ptr)
216 stdcall EnumerateTraceGuids(ptr long ptr) ntdll.EtwEnumerateTraceGuids
217 stdcall EqualDomainSid(ptr ptr ptr)
218 stdcall EqualPrefixSid(ptr ptr)
219 stdcall EqualSid(ptr ptr)
220 stdcall FileEncryptionStatusA(str ptr)
221 stdcall FileEncryptionStatusW(wstr ptr)
222 stdcall FindFirstFreeAce(ptr ptr)
223 stdcall FlushTraceA(double str ptr) ntdll.EtwFlushTraceA
224 stdcall FlushTraceW(double wstr ptr) ntdll.EtwFlushTraceW
225 stub FreeEncryptedFileKeyInfo
226 stdcall FreeEncryptionCertificateHashList(ptr)
227 stdcall FreeInheritedFromArray(ptr long ptr)
228 stdcall FreeSid(ptr)
229 stub GetAccessPermissionsForObjectA
230 stub GetAccessPermissionsForObjectW
231 stdcall GetAce(ptr long ptr)
232 stdcall GetAclInformation(ptr ptr long long)
233 stdcall GetAuditedPermissionsFromAclA(ptr ptr ptr ptr)
234 stdcall GetAuditedPermissionsFromAclW(ptr ptr ptr ptr)
235 stdcall GetCurrentHwProfileA(ptr)
236 stdcall GetCurrentHwProfileW(ptr)
237 stdcall GetEffectiveRightsFromAclA(ptr ptr ptr)
238 stdcall GetEffectiveRightsFromAclW(ptr ptr ptr)
239 stdcall GetEventLogInformation(long long ptr long ptr)
240 stdcall GetExplicitEntriesFromAclA(ptr ptr ptr) advapi32.GetExplicitEntriesFromAclW
241 stdcall GetExplicitEntriesFromAclW(ptr ptr ptr)
242 stdcall GetFileSecurityA(str long ptr long ptr)
243 stdcall GetFileSecurityW(wstr long ptr long ptr)
244 stub GetInformationCodeAuthzLevelW
245 stub GetInformationCodeAuthzPolicyW
246 stdcall GetInheritanceSourceA(str long long long ptr long ptr ptr ptr ptr)
247 stdcall GetInheritanceSourceW(wstr long long long ptr long ptr ptr ptr ptr)
248 stdcall GetKernelObjectSecurity(long long ptr long ptr)
249 stdcall GetLengthSid(ptr)
250 stub GetLocalManagedApplicationData
251 stub GetLocalManagedApplications
252 stub GetManagedApplicationCategories
253 stub GetManagedApplications
254 stdcall GetMultipleTrusteeA(ptr)
255 stdcall GetMultipleTrusteeOperationA(ptr)
256 stdcall GetMultipleTrusteeOperationW(ptr)
257 stdcall GetMultipleTrusteeW(ptr)
258 stdcall GetNamedSecurityInfoA(str long long ptr ptr ptr ptr ptr)
259 stub GetNamedSecurityInfoExA
260 stub GetNamedSecurityInfoExW
261 stdcall GetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr ptr)
262 stdcall GetNumberOfEventLogRecords(long ptr)
263 stdcall GetOldestEventLogRecord(long ptr)
264 stub GetOverlappedAccessResults
265 stdcall GetPrivateObjectSecurity(ptr long ptr long ptr)
266 stdcall GetSecurityDescriptorControl(ptr ptr ptr)
267 stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr)
268 stdcall GetSecurityDescriptorGroup(ptr ptr ptr)
269 stdcall GetSecurityDescriptorLength(ptr) ntdll.RtlLengthSecurityDescriptor
270 stdcall GetSecurityDescriptorOwner(ptr ptr ptr)
271 stdcall GetSecurityDescriptorRMControl(ptr ptr)
272 stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr)
273 stdcall GetSecurityInfo(long long long ptr ptr ptr ptr ptr)
274 stdcall GetSecurityInfoExA(long long long str str ptr ptr ptr ptr)
275 stdcall GetSecurityInfoExW(long long long wstr wstr ptr ptr ptr ptr)
276 stdcall GetServiceDisplayNameA(ptr str ptr ptr)
277 stdcall GetServiceDisplayNameW(ptr wstr ptr ptr)
278 stdcall GetServiceKeyNameA(long str ptr ptr)
279 stdcall GetServiceKeyNameW(long wstr ptr ptr)
280 stdcall GetSidIdentifierAuthority(ptr)
281 stdcall GetSidLengthRequired(long)
282 stdcall GetSidSubAuthority(ptr long)
283 stdcall GetSidSubAuthorityCount(ptr)
284 stdcall GetTokenInformation(long long ptr long ptr)
285 stdcall GetTraceEnableFlags(double) ntdll.EtwGetTraceEnableFlags
286 stdcall GetTraceEnableLevel(double) ntdll.EtwGetTraceEnableLevel
287 stdcall GetTraceLoggerHandle(ptr) ntdll.EtwGetTraceLoggerHandle
288 stdcall GetTrusteeFormA(ptr)
289 stdcall GetTrusteeFormW(ptr)
290 stdcall GetTrusteeNameA(ptr)
291 stdcall GetTrusteeNameW(ptr)
292 stdcall GetTrusteeTypeA(ptr)
293 stdcall GetTrusteeTypeW(ptr)
294 stdcall GetUserNameA(ptr ptr)
295 stdcall GetUserNameW(ptr ptr)
296 stdcall GetWindowsAccountDomainSid(ptr ptr ptr)
297 stub I_QueryTagInformation
298 stub I_ScIsSecurityProcess
299 stub I_ScPnPGetServiceName
300 stub I_ScSendTSMessage
301 stdcall I_ScSetServiceBitsA(ptr long long long str)
302 stdcall I_ScSetServiceBitsW(ptr long long long wstr)
303 stub IdentifyCodeAuthzLevelW
304 stdcall ImpersonateAnonymousToken(ptr)
305 stdcall ImpersonateLoggedOnUser(long)
306 stdcall ImpersonateNamedPipeClient(long)
307 stdcall ImpersonateSelf(long)
308 stdcall InitializeAcl(ptr long long)
309 stdcall InitializeSecurityDescriptor(ptr long)
310 stdcall InitializeSid(ptr ptr long)
311 stdcall InitiateSystemShutdownA(str str long long long)
312 stdcall InitiateSystemShutdownExA(str str long long long long)
313 stdcall InitiateSystemShutdownExW(wstr wstr long long long long)
314 stdcall InitiateSystemShutdownW(str str long long long)
315 stub InstallApplication
316 stdcall IsTextUnicode(ptr long ptr) ntdll.RtlIsTextUnicode
317 stdcall IsTokenRestricted(long)
318 stub IsTokenUntrusted
319 stdcall IsValidAcl(ptr)
320 stdcall IsValidSecurityDescriptor(ptr)
321 stdcall IsValidSid(ptr)
322 stdcall IsWellKnownSid(ptr long)
323 stdcall LockServiceDatabase(ptr)
324 stdcall LogonUserA(str str str long long ptr)
325 stub LogonUserExA
326 stub LogonUserExW
327 stdcall LogonUserW(wstr wstr wstr long long ptr)
328 stdcall LookupAccountNameA(str str ptr ptr ptr ptr ptr)
329 stdcall LookupAccountNameW(wstr wstr ptr ptr ptr ptr ptr)
330 stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr)
331 stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr)
332 stdcall LookupPrivilegeDisplayNameA(str str str ptr ptr)
333 stdcall LookupPrivilegeDisplayNameW(wstr wstr wstr ptr ptr)
334 stdcall LookupPrivilegeNameA(str ptr ptr long)
335 stdcall LookupPrivilegeNameW(wstr ptr ptr long)
336 stdcall LookupPrivilegeValueA(ptr ptr ptr)
337 stdcall LookupPrivilegeValueW(ptr ptr ptr)
338 stub LookupSecurityDescriptorPartsA
339 stub LookupSecurityDescriptorPartsW
340 stdcall LsaAddAccountRights(ptr ptr ptr long)
341 stdcall LsaAddPrivilegesToAccount(ptr ptr)
342 stdcall LsaClearAuditLog(ptr)
343 stdcall LsaClose(ptr)
344 stdcall LsaCreateAccount(ptr ptr long ptr)
345 stdcall LsaCreateSecret(ptr ptr long ptr)
346 stdcall LsaCreateTrustedDomain(ptr ptr long ptr)
347 stdcall LsaCreateTrustedDomainEx(ptr ptr ptr long ptr)
348 stdcall LsaDelete(ptr)
349 stdcall LsaDeleteTrustedDomain(ptr ptr)
350 stdcall LsaEnumerateAccountRights(ptr ptr ptr ptr)
351 stdcall LsaEnumerateAccounts(ptr ptr ptr long ptr)
352 stdcall LsaEnumerateAccountsWithUserRight(ptr ptr ptr ptr)
353 stdcall LsaEnumeratePrivileges(ptr ptr ptr long ptr)
354 stdcall LsaEnumeratePrivilegesOfAccount(ptr ptr)
355 stdcall LsaEnumerateTrustedDomains(ptr ptr ptr long ptr)
356 stdcall LsaEnumerateTrustedDomainsEx(ptr ptr ptr long ptr)
357 stdcall LsaFreeMemory(ptr)
358 stdcall LsaGetQuotasForAccount(ptr ptr)
359 stdcall LsaGetRemoteUserName(ptr ptr ptr)
360 stdcall LsaGetSystemAccessAccount(ptr ptr)
361 stdcall LsaGetUserName(ptr ptr)
362 stub LsaICLookupNames
363 stub LsaICLookupNamesWithCreds
364 stub LsaICLookupSids
365 stub LsaICLookupSidsWithCreds
366 stdcall LsaLookupNames2(ptr long long ptr ptr ptr)
367 stdcall LsaLookupNames(ptr long ptr ptr ptr)
368 stdcall LsaLookupPrivilegeDisplayName(ptr ptr ptr ptr)
369 stdcall LsaLookupPrivilegeName(ptr ptr ptr)
370 stdcall LsaLookupPrivilegeValue(ptr ptr ptr)
371 stdcall LsaLookupSids(ptr long ptr ptr ptr)
372 stdcall LsaNtStatusToWinError(long)
373 stdcall LsaOpenAccount(ptr ptr long ptr)
374 stdcall LsaOpenPolicy(ptr ptr long ptr)
375 stdcall LsaOpenPolicySce(ptr ptr long ptr)
376 stdcall LsaOpenSecret(ptr ptr long ptr)
377 stdcall LsaOpenTrustedDomain(ptr ptr long ptr)
378 stdcall LsaOpenTrustedDomainByName(ptr ptr long ptr)
379 stdcall LsaQueryDomainInformationPolicy(ptr long ptr)
380 stdcall LsaQueryForestTrustInformation(ptr ptr ptr)
381 stdcall LsaQueryInfoTrustedDomain(ptr long ptr)
382 stdcall LsaQueryInformationPolicy(ptr long ptr)
383 stdcall LsaQuerySecret(ptr ptr ptr ptr ptr)
384 stdcall LsaQuerySecurityObject(ptr long ptr)
385 stdcall LsaQueryTrustedDomainInfo(ptr ptr long ptr)
386 stdcall LsaQueryTrustedDomainInfoByName(ptr ptr long ptr)
387 stdcall LsaRemoveAccountRights(ptr ptr long ptr long)
388 stdcall LsaRemovePrivilegesFromAccount(ptr long ptr)
389 stdcall LsaRetrievePrivateData(ptr ptr ptr)
390 stdcall LsaSetDomainInformationPolicy(ptr long ptr)
391 stdcall LsaSetForestTrustInformation(ptr ptr ptr long ptr)
392 stdcall LsaSetInformationPolicy(ptr long ptr)
393 stdcall LsaSetInformationTrustedDomain(ptr long ptr)
394 stdcall LsaSetQuotasForAccount(ptr ptr)
395 stdcall LsaSetSecret(ptr ptr ptr)
396 stdcall LsaSetSecurityObject(ptr long ptr)
397 stdcall LsaSetSystemAccessAccount(ptr long)
398 stdcall LsaSetTrustedDomainInfoByName(ptr ptr long ptr)
399 stdcall LsaSetTrustedDomainInformation(ptr ptr long ptr)
400 stdcall LsaStorePrivateData(ptr ptr ptr)
401 stdcall MD4Final(ptr)
402 stdcall MD4Init(ptr)
403 stdcall MD4Update(ptr ptr long)
404 stdcall MD5Final(ptr)
405 stdcall MD5Init(ptr)
406 stdcall MD5Update(ptr ptr long)
407 stub MSChapSrvChangePassword2
408 stub MSChapSrvChangePassword
409 stdcall MakeAbsoluteSD2(ptr ptr)
410 stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
411 stdcall MakeSelfRelativeSD(ptr ptr ptr)
412 stdcall MapGenericMask(ptr ptr) ntdll.RtlMapGenericMask
413 stdcall NotifyBootConfigStatus(long)
414 stdcall NotifyChangeEventLog(long long)
415 stdcall ObjectCloseAuditAlarmA(str ptr long)
416 stdcall ObjectCloseAuditAlarmW(wstr ptr long)
417 stdcall ObjectDeleteAuditAlarmA(str ptr long)
418 stdcall ObjectDeleteAuditAlarmW(wstr ptr long)
419 stdcall ObjectOpenAuditAlarmA(str ptr str str ptr long long long ptr long long ptr)
420 stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr)
421 stdcall ObjectPrivilegeAuditAlarmA(str ptr long long ptr long)
422 stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long)
423 stdcall OpenBackupEventLogA(str str)
424 stdcall OpenBackupEventLogW(wstr wstr)
425 stub OpenEncryptedFileRawA
426 stub OpenEncryptedFileRawW
427 stdcall OpenEventLogA(str str)
428 stdcall OpenEventLogW(wstr wstr)
429 stdcall OpenProcessToken(long long ptr)
430 stdcall OpenSCManagerA(str str long)
431 stdcall OpenSCManagerW(wstr wstr long)
432 stdcall OpenServiceA(long str long)
433 stdcall OpenServiceW(long wstr long)
434 stdcall OpenThreadToken(long long long ptr)
435 stub OpenTraceA
436 stub OpenTraceW
437 stdcall PrivilegeCheck(ptr ptr ptr)
438 stdcall PrivilegedServiceAuditAlarmA(str str long ptr long)
439 stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long)
440 stub ProcessIdleTasks
441 stub ProcessTrace
442 stdcall QueryAllTracesA(ptr long ptr) ntdll.EtwQueryAllTracesA
443 stdcall QueryAllTracesW(ptr long ptr) ntdll.EtwQueryAllTracesW
444 stdcall QueryRecoveryAgentsOnEncryptedFile(wstr ptr)
445 stdcall QueryServiceConfig2A(long long ptr long ptr)
446 stdcall QueryServiceConfig2W(long long ptr long ptr)
447 stdcall QueryServiceConfigA(long ptr long ptr)
448 stdcall QueryServiceConfigW(long ptr long ptr)
449 stdcall QueryServiceLockStatusA(long ptr long ptr)
450 stdcall QueryServiceLockStatusW(long ptr long ptr)
451 stdcall QueryServiceObjectSecurity(long long ptr long ptr)
452 stdcall QueryServiceStatus(long ptr)
453 stdcall QueryServiceStatusEx(long long ptr long ptr)
454 stdcall QueryTraceA(double str ptr) ntdll.EtwQueryTraceA
455 stdcall QueryTraceW(double str ptr) ntdll.EtwQueryTraceA
456 stdcall QueryUsersOnEncryptedFile(wstr ptr)
457 stub ReadEncryptedFileRaw
458 stdcall ReadEventLogA(long long long ptr long ptr ptr)
459 stdcall ReadEventLogW(long long long ptr long ptr ptr)
460 stdcall RegCloseKey(long)
461 stdcall RegConnectRegistryA(str long ptr)
462 stub RegConnectRegistryExA
463 stub RegConnectRegistryExW
464 stdcall RegConnectRegistryW(wstr long ptr)
465 stdcall RegCreateKeyA(long str ptr)
466 stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr)
467 stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr)
468 stdcall RegCreateKeyW(long wstr ptr)
469 stdcall RegDeleteKeyA(long str)
470 stdcall RegDeleteKeyExA(long str long long)
471 stdcall RegDeleteKeyExW(long wstr long long)
472 stdcall RegDeleteKeyW(long wstr)
473 stdcall RegDeleteValueA(long str)
474 stdcall RegDeleteValueW(long wstr)
475 stdcall RegDisablePredefinedCache()
476 stdcall RegDisableReflectionKey(ptr)
477 stdcall RegEnableReflectionKey(ptr)
478 stdcall RegEnumKeyA(long long ptr long)
479 stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr)
480 stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr)
481 stdcall RegEnumKeyW(long long ptr long)
482 stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr)
483 stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr)
484 stdcall RegFlushKey(long)
485 stdcall RegGetKeySecurity(long long ptr ptr)
486 stdcall RegGetValueA(long str str long ptr ptr ptr)
487 stdcall RegGetValueW(long wstr wstr long ptr ptr ptr)
488 stdcall RegLoadKeyA(long str str)
489 stdcall RegLoadKeyW(long wstr wstr)
490 stdcall RegNotifyChangeKeyValue(long long long long long)
491 stdcall RegOpenCurrentUser(long ptr)
492 stdcall RegOpenKeyA(long str ptr)
493 stdcall RegOpenKeyExA(long str long long ptr)
494 stdcall RegOpenKeyExW(long wstr long long ptr)
495 stdcall RegOpenKeyW(long wstr ptr)
496 stdcall RegOpenUserClassesRoot(ptr long long ptr)
497 stdcall RegOverridePredefKey(long long)
498 stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
499 stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
500 stdcall RegQueryMultipleValuesA(long ptr long ptr ptr)
501 stdcall RegQueryMultipleValuesW(long ptr long ptr ptr)
502 stdcall RegQueryReflectionKey(ptr ptr)
503 stdcall RegQueryValueA(long str ptr ptr)
504 stdcall RegQueryValueExA(long str ptr ptr ptr ptr)
505 stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr)
506 stdcall RegQueryValueW(long wstr ptr ptr)
507 stdcall RegReplaceKeyA(long str str str)
508 stdcall RegReplaceKeyW(long wstr wstr wstr)
509 stdcall RegRestoreKeyA(long str long)
510 stdcall RegRestoreKeyW(long wstr long)
511 stdcall RegSaveKeyA(long ptr ptr)
512 stdcall RegSaveKeyExA(long str ptr long)
513 stdcall RegSaveKeyExW(long str ptr long)
514 stdcall RegSaveKeyW(long ptr ptr)
515 stdcall RegSetKeySecurity(long long ptr)
516 stdcall RegSetValueA(long str long ptr long)
517 stdcall RegSetValueExA(long str long long ptr long)
518 stdcall RegSetValueExW(long wstr long long ptr long)
519 stdcall RegSetValueW(long wstr long ptr long)
520 stdcall RegUnLoadKeyA(long str)
521 stdcall RegUnLoadKeyW(long wstr)
522 stdcall RegisterEventSourceA(ptr ptr)
523 stdcall RegisterEventSourceW(ptr ptr)
524 stub RegisterIdleTask
525 stdcall RegisterServiceCtrlHandlerA(str ptr)
526 stdcall RegisterServiceCtrlHandlerExA(str ptr ptr)
527 stdcall RegisterServiceCtrlHandlerExW(wstr ptr ptr)
528 stdcall RegisterServiceCtrlHandlerW(wstr ptr)
529 stdcall RegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr) ntdll.EtwRegisterTraceGuidsA
530 stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) ntdll.EtwRegisterTraceGuidsW
531 stub RemoveTraceCallback
532 stdcall RemoveUsersFromEncryptedFile(wstr ptr)
533 stdcall ReportEventA(long long long long ptr long long str ptr)
534 stdcall ReportEventW(long long long long ptr long long wstr ptr)
535 stdcall RevertToSelf()
536 stdcall SaferCloseLevel(ptr)
537 stdcall SaferComputeTokenFromLevel(ptr ptr ptr long ptr)
538 stdcall SaferCreateLevel(long long long ptr ptr)
539 stub SaferGetLevelInformation
540 stdcall SaferGetPolicyInformation(long long long ptr ptr ptr)
541 stdcall SaferIdentifyLevel(long ptr ptr ptr)
542 stdcall SaferRecordEventLogEntry(ptr wstr ptr)
543 stub SaferSetLevelInformation
544 stub SaferSetPolicyInformation
545 stub SaferiChangeRegistryScope
546 stub SaferiCompareTokenLevels
547 stub SaferiIsExecutableFileType
548 stub SaferiPopulateDefaultsInRegistry
549 stub SaferiRecordEventLogEntry
550 stub SaferiReplaceProcessThreadTokens
551 stub SaferiSearchMatchingHashRules
552 stdcall SetAclInformation(ptr ptr long long)
553 stub SetEntriesInAccessListA
554 stub SetEntriesInAccessListW
555 stdcall SetEntriesInAclA(long ptr ptr ptr)
556 stdcall SetEntriesInAclW(long ptr ptr ptr)
557 stub SetEntriesInAuditListA
558 stub SetEntriesInAuditListW
559 stdcall SetFileSecurityA(str long ptr)
560 stdcall SetFileSecurityW(wstr long ptr)
561 stub SetInformationCodeAuthzLevelW
562 stub SetInformationCodeAuthzPolicyW
563 stdcall SetKernelObjectSecurity(long long ptr)
564 stdcall SetNamedSecurityInfoA(str long ptr ptr ptr ptr ptr)
565 stub SetNamedSecurityInfoExA
566 stub SetNamedSecurityInfoExW
567 stdcall SetNamedSecurityInfoW(wstr long ptr ptr ptr ptr ptr)
568 stdcall SetPrivateObjectSecurity(long ptr ptr ptr long)
569 stub SetPrivateObjectSecurityEx
570 stdcall SetSecurityDescriptorControl(ptr long long)
571 stdcall SetSecurityDescriptorDacl(ptr long ptr long)
572 stdcall SetSecurityDescriptorGroup(ptr ptr long)
573 stdcall SetSecurityDescriptorOwner(ptr ptr long)
574 stdcall SetSecurityDescriptorRMControl(ptr ptr)
575 stdcall SetSecurityDescriptorSacl(ptr long ptr long)
576 stdcall SetSecurityInfo(long long long ptr ptr ptr ptr)
577 stub SetSecurityInfoExA
578 stub SetSecurityInfoExW
579 stdcall SetServiceBits(long long long long)
580 stdcall SetServiceObjectSecurity(long long ptr)
581 stdcall SetServiceStatus(long long)
582 stdcall SetThreadToken(ptr ptr)
583 stdcall SetTokenInformation(long long ptr long)
584 stub SetTraceCallback
585 stub SetUserFileEncryptionKey
586 stdcall StartServiceA(long long ptr)
587 stdcall StartServiceCtrlDispatcherA(ptr)
588 stdcall StartServiceCtrlDispatcherW(ptr)
589 stdcall StartServiceW(long long ptr)
590 stdcall StartTraceA(ptr str ptr) ntdll.EtwStartTraceA
591 stdcall StartTraceW(ptr wstr ptr) ntdll.EtwStartTraceW
592 stdcall StopTraceA(double str ptr) ntdll.EtwStopTraceA
593 stdcall StopTraceW(double wstr ptr) ntdll.EtwStopTraceA
594 stdcall SystemFunction001(ptr ptr ptr)
595 stdcall SystemFunction002(ptr ptr ptr)
596 stdcall SystemFunction003(ptr ptr)
597 stdcall SystemFunction004(ptr ptr ptr)
598 stdcall SystemFunction005(ptr ptr ptr)
599 stdcall SystemFunction006(ptr ptr)
600 stdcall SystemFunction007(ptr ptr)
601 stdcall SystemFunction008(ptr ptr ptr)
602 stdcall SystemFunction009(ptr ptr ptr)
603 stdcall SystemFunction010(ptr ptr ptr)
604 stdcall SystemFunction011(ptr ptr ptr) SystemFunction010
605 stdcall SystemFunction012(ptr ptr ptr)
606 stdcall SystemFunction013(ptr ptr ptr)
607 stdcall SystemFunction014(ptr ptr ptr) SystemFunction012
608 stdcall SystemFunction015(ptr ptr ptr) SystemFunction013
609 stdcall SystemFunction016(ptr ptr ptr) SystemFunction012
610 stdcall SystemFunction017(ptr ptr ptr) SystemFunction013
611 stdcall SystemFunction018(ptr ptr ptr) SystemFunction012
612 stdcall SystemFunction019(ptr ptr ptr) SystemFunction013
613 stdcall SystemFunction020(ptr ptr ptr) SystemFunction012
614 stdcall SystemFunction021(ptr ptr ptr) SystemFunction013
615 stdcall SystemFunction022(ptr ptr ptr) SystemFunction012
616 stdcall SystemFunction023(ptr ptr ptr) SystemFunction013
617 stdcall SystemFunction024(ptr ptr ptr)
618 stdcall SystemFunction025(ptr ptr ptr)
619 stdcall SystemFunction026(ptr ptr ptr) SystemFunction024
620 stdcall SystemFunction027(ptr ptr ptr) SystemFunction025
621 stdcall SystemFunction028(long long)
622 stdcall SystemFunction029(long long)
623 stdcall SystemFunction030(ptr ptr)
624 stdcall SystemFunction031(ptr ptr) SystemFunction030
625 stdcall SystemFunction032(ptr ptr)
626 stdcall SystemFunction033(long long)
627 stdcall SystemFunction034(long long)
628 stdcall SystemFunction035(str)
629 stdcall SystemFunction036(ptr long) # RtlGenRandom
630 stdcall SystemFunction040(ptr long long) # RtlEncryptMemory
631 stdcall SystemFunction041(ptr long long) # RtlDecryptMemory
632 stdcall TraceEvent(double ptr) ntdll.EtwTraceEvent
633 stdcall TraceEventInstance(double ptr ptr ptr) ntdll.EtwTraceEventInstance
634 varargs TraceMessage(ptr long ptr long) ntdll.EtwTraceMessage
635 stdcall TraceMessageVa(double long ptr long ptr) ntdll.EtwTraceMessageVa
636 stdcall TreeResetNamedSecurityInfoA(str ptr ptr ptr ptr ptr ptr long ptr ptr ptr)
637 stdcall TreeResetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr long ptr long ptr)
638 stub TrusteeAccessToObjectA
639 stub TrusteeAccessToObjectW
640 stub UninstallApplication
641 stdcall UnlockServiceDatabase(ptr)
642 stub UnregisterIdleTask
643 stdcall UnregisterTraceGuids(double) ntdll.EtwUnregisterTraceGuids
644 stdcall UpdateTraceA(double str ptr) ntdll.EtwUpdateTraceA
645 stdcall UpdateTraceW(double wstr ptr) ntdll.EtwUpdateTraceW
646 stub WdmWmiServiceMain
647 stub WmiCloseBlock
648 stub WmiCloseTraceWithCursor
649 stub WmiConvertTimestamp
650 stub WmiDevInstToInstanceNameA
651 stub WmiDevInstToInstanceNameW
652 stub WmiEnumerateGuids
653 stub WmiExecuteMethodA
654 stub WmiExecuteMethodW
655 stub WmiFileHandleToInstanceNameA
656 stub WmiFileHandleToInstanceNameW
657 stub WmiFreeBuffer
658 stub WmiGetFirstTraceOffset
659 stub WmiGetNextEvent
660 stub WmiGetTraceHeader
661 stub WmiMofEnumerateResourcesA
662 stub WmiMofEnumerateResourcesW
663 stdcall WmiNotificationRegistrationA(ptr long ptr long long) ntdll.EtwNotificationRegistrationA
664 stdcall WmiNotificationRegistrationW(ptr long ptr long long) ntdll.EtwNotificationRegistrationW
665 stub WmiOpenBlock
666 stub WmiOpenTraceWithCursor
667 stub WmiParseTraceEvent
668 stub WmiQueryAllDataA
669 stub WmiQueryAllDataMultipleA
670 stub WmiQueryAllDataMultipleW
671 stub WmiQueryAllDataW
672 stub WmiQueryGuidInformation
673 stub WmiQuerySingleInstanceA
674 stub WmiQuerySingleInstanceMultipleA
675 stub WmiQuerySingleInstanceMultipleW
676 stub WmiQuerySingleInstanceW
677 stdcall WmiReceiveNotificationsA() ntdll.EtwReceiveNotificationsA # FIXME prototype
678 stdcall WmiReceiveNotificationsW() ntdll.EtwReceiveNotificationsW # FIXME prototype
679 stub WmiSetSingleInstanceA
680 stub WmiSetSingleInstanceW
681 stub WmiSetSingleItemA
682 stub WmiSetSingleItemW
683 stub Wow64Win32ApiEntry
684 stdcall WriteEncryptedFileRaw(ptr ptr ptr)
