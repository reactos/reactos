 @ stdcall AuthzAccessCheck(long ptr ptr ptr ptr ptr long ptr ptr)
 @ stdcall AuthzAddSidsToContext(ptr ptr long ptr long ptr)
 @ stdcall AuthzCachedAccessCheck(long ptr ptr ptr ptr)
 @ stdcall AuthzEnumerateSecurityEventSources(long ptr ptr ptr)
 @ stdcall AuthzFreeAuditEvent(ptr)
 @ stdcall AuthzFreeContext(ptr)
 @ stdcall AuthzFreeHandle(ptr)
 @ stdcall AuthzFreeResourceManager(ptr)
 @ stdcall AuthzGetInformationFromContext(ptr ptr long ptr ptr)
 @ stdcall AuthzInitializeContextFromAuthzContext(long ptr ptr double ptr ptr)
 @ stdcall AuthzInitializeContextFromSid(long ptr ptr ptr double ptr ptr)
 @ stdcall AuthzInitializeContextFromToken(long ptr ptr ptr double ptr ptr)
 @ stdcall AuthzInitializeObjectAccessAuditEvent(long ptr wstr wstr wstr wstr ptr long)
 @ stdcall AuthzInitializeObjectAccessAuditEvent2(long ptr wstr wstr wstr wstr wstr ptr long)
 @ stdcall AuthzInitializeResourceManager(long ptr ptr ptr ptr ptr)
 @ stdcall AuthzInstallSecurityEventSource(long ptr)
 @ stdcall AuthzOpenObjectAudit(long ptr ptr ptr ptr ptr long ptr)
 @ stdcall AuthzRegisterSecurityEventSource(long ptr ptr)
 @ varargs AuthzReportSecurityEvent(long ptr long ptr long)
 @ stdcall AuthzReportSecurityEventFromParams(long ptr long ptr ptr)
 @ stdcall AuthzUninstallSecurityEventSource(long ptr)
 @ stdcall AuthzUnregisterSecurityEventSource(long ptr)
#AuthziAllocateAuditParams
#AuthziFreeAuditEventType
#AuthziFreeAuditParams
#AuthziFreeAuditQueue
#AuthziInitializeAuditEvent
#AuthziInitializeAuditEventType
#AuthziInitializeAuditParams
#AuthziInitializeAuditParamsFromArray
#AuthziInitializeAuditParamsWithRM
#AuthziInitializeAuditQueue
#AuthziInitializeContextFromSid
#AuthziLogAuditEvent
#AuthziModifyAuditEvent2
#AuthziModifyAuditEvent
#AuthziModifyAuditEventType
#AuthziModifyAuditQueue
#AuthziQueryAuditPolicy
#AuthziSetAuditPolicy
#AuthziSourceAudit

# EOF
