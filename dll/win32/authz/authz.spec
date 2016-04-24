1 stdcall AuthzAccessCheck(long ptr ptr ptr ptr ptr long ptr ptr)
2 stdcall AuthzAddSidsToContext(ptr ptr long ptr long ptr)
3 stdcall AuthzCachedAccessCheck(long ptr ptr ptr ptr)
4 stdcall AuthzEnumerateSecurityEventSources(long ptr ptr ptr)
5 stdcall AuthzFreeAuditEvent(ptr)
6 stdcall AuthzFreeContext(ptr)
7 stdcall AuthzFreeHandle(ptr)
8 stdcall AuthzFreeResourceManager(ptr)
9 stdcall AuthzGetInformationFromContext(ptr ptr long ptr ptr)
10 stdcall AuthzInitializeContextFromAuthzContext(long ptr ptr long long ptr ptr)
11 stdcall AuthzInitializeContextFromSid(long ptr ptr ptr long long ptr ptr)
12 stdcall AuthzInitializeContextFromToken(long ptr ptr ptr long long ptr ptr)
13 cdecl AuthzInitializeObjectAccessAuditEvent(long ptr wstr wstr wstr wstr ptr long)
14 cdecl AuthzInitializeObjectAccessAuditEvent2(long ptr wstr wstr wstr wstr wstr ptr long)
15 stdcall AuthzInitializeResourceManager(long ptr ptr ptr wstr ptr)
16 stdcall AuthzInstallSecurityEventSource(long ptr)
17 stdcall AuthzOpenObjectAudit(long ptr ptr ptr ptr ptr long ptr)
18 stdcall AuthzRegisterSecurityEventSource(long ptr ptr)
19 cdecl AuthzReportSecurityEvent(long ptr long ptr long) authz.AuthzReportSecurityEvent
20 stdcall AuthzReportSecurityEventFromParams(long ptr long ptr ptr)
21 stdcall AuthzUninstallSecurityEventSource(long wstr)
22 stdcall AuthzUnregisterSecurityEventSource(long ptr)
23 stub AuthziAllocateAuditParams
24 stub AuthziFreeAuditEventType
25 stub AuthziFreeAuditParams
26 stub AuthziFreeAuditQueue
27 stub AuthziInitializeAuditEvent
28 stub AuthziInitializeAuditEventType
29 stub AuthziInitializeAuditParams
30 stub AuthziInitializeAuditParamsFromArray
31 stub AuthziInitializeAuditParamsWithRM
32 stub AuthziInitializeAuditQueue
33 stub AuthziInitializeContextFromSid
34 stub AuthziLogAuditEvent
35 stub AuthziModifyAuditEvent2
36 stub AuthziModifyAuditEvent
37 stub AuthziModifyAuditEventType
38 stub AuthziModifyAuditQueue
39 stub AuthziQueryAuditPolicy
40 stub AuthziSetAuditPolicy
41 stub AuthziSourceAudit
