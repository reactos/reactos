@ stdcall CloseTrace(int64) advapi32.CloseTrace
@ stdcall ControlTraceA(int64 str ptr long) advapi32.ControlTraceA
@ stdcall ControlTraceW(int64 wstr ptr long) advapi32.ControlTraceW
@ stdcall CreateTraceInstanceId(long ptr) advapi32.CreateTraceInstanceId
@ stdcall EnableTrace(long long long ptr int64) advapi32.EnableTrace
@ stdcall GetTraceEnableFlags(int64) advapi32.GetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) advapi32.GetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) advapi32.GetTraceLoggerHandle
@ stdcall -ret64 OpenTraceA(ptr) advapi32.OpenTraceA
@ stdcall -ret64 OpenTraceW(ptr) advapi32.OpenTraceW
@ stdcall ProcessTrace(ptr long ptr ptr) advapi32.ProcessTrace
@ stdcall QueryAllTracesA(ptr long ptr) advapi32.QueryAllTracesA
@ stdcall QueryAllTracesW(ptr long ptr) advapi32.QueryAllTracesW
@ stdcall RegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr) advapi32.RegisterTraceGuidsA
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) advapi32.RegisterTraceGuidsW
@ stdcall RemoveTraceCallback(ptr) advapi32.RemoveTraceCallback
@ stdcall SetTraceCallback(ptr ptr) advapi32.SetTraceCallback
@ stdcall StartTraceA(ptr str ptr) advapi32.StartTraceA
@ stdcall StartTraceW(ptr wstr ptr) advapi32.StartTraceW
@ stdcall TraceEvent(int64 ptr) advapi32.TraceEvent
@ stdcall TraceEventInstance(int64 ptr ptr ptr) advapi32.TraceEventInstance
@ stdcall UnregisterTraceGuids(int64) advapi32.UnregisterTraceGuids
@ stdcall WmiCloseBlock() advapi32.WmiCloseBlock
@ stdcall WmiDevInstToInstanceNameA() advapi32.WmiDevInstToInstanceNameA
@ stdcall WmiDevInstToInstanceNameW() advapi32.WmiDevInstToInstanceNameW
@ stdcall WmiEnumerateGuids() advapi32.WmiEnumerateGuids
@ stdcall WmiExecuteMethodA() advapi32.WmiExecuteMethodA
@ stdcall WmiExecuteMethodW() advapi32.WmiExecuteMethodW
@ stdcall WmiFileHandleToInstanceNameA() advapi32.WmiFileHandleToInstanceNameA
@ stdcall WmiFileHandleToInstanceNameW() advapi32.WmiFileHandleToInstanceNameW
@ stdcall WmiFreeBuffer() advapi32.WmiFreeBuffer
@ stdcall WmiMofEnumerateResourcesA() advapi32.WmiMofEnumerateResourcesA
@ stdcall WmiMofEnumerateResourcesW() advapi32.WmiMofEnumerateResourcesW
@ stdcall WmiNotificationRegistrationA() advapi32.WmiNotificationRegistrationA
@ stdcall WmiNotificationRegistrationW() advapi32.WmiNotificationRegistrationW
@ stdcall WmiOpenBlock() advapi32.WmiOpenBlock
@ stdcall WmiQueryAllDataA() advapi32.WmiQueryAllDataA
@ stdcall WmiQueryAllDataW() advapi32.WmiQueryAllDataW
@ stdcall WmiQueryGuidInformation() advapi32.WmiQueryGuidInformation
@ stdcall WmiQuerySingleInstanceA() advapi32.WmiQuerySingleInstanceA
@ stdcall WmiQuerySingleInstanceW() advapi32.WmiQuerySingleInstanceW
@ stdcall WmiSetSingleInstanceA() advapi32.WmiSetSingleInstanceA
@ stdcall WmiSetSingleInstanceW() advapi32.WmiSetSingleInstanceW
@ stdcall WmiSetSingleItemA() advapi32.WmiSetSingleItemA
@ stdcall WmiSetSingleItemW() advapi32.WmiSetSingleItemW
