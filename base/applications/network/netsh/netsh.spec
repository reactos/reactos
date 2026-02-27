@ stub ConvertGuidToString
@ stub ConvertStringToGuid
@ stub DisplayMessageM
@ stub DisplayMessageToConsole
@ stdcall FreeQuotedString(wstr)
@ stdcall FreeString(wstr)
@ stub GenericMonitor
@ stub GetEnumString
@ stub GetHostMachineInfo
@ stub InitializeConsole
@ stdcall MakeQuotedString(wstr)
@ varargs MakeString(ptr long)
@ stub MatchCmdLine
@ stdcall MatchEnumTag(ptr wstr long ptr ptr)
@ stub MatchTagsInCmdLine
@ stdcall MatchToken(wstr wstr)
@ stdcall NsGetFriendlyNameFromIfName(long wstr ptr ptr)
@ stub NsGetIfNameFromFriendlyName
@ stdcall PreprocessCommand(ptr ptr long long ptr long long long ptr)
@ varargs PrintError(ptr long)
@ stub PrintErrorLog
@ varargs PrintMessage(wstr)
@ varargs PrintMessageFromModule(ptr long)
@ stub RefreshConsole
@ stdcall RegisterContext(ptr)
@ stdcall RegisterHelper(ptr ptr)
