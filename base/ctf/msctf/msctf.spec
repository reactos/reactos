@ stdcall TF_GetLangIcon(long ptr long)
@ stdcall TF_GetMlngHKL(long ptr ptr long)
@ stdcall TF_GetMlngIconIndex(long)
@ stdcall TF_GetThreadFlags(long ptr ptr ptr)
@ stdcall TF_InatExtractIcon(long)
@ stdcall TF_InitMlngInfo()
@ stdcall TF_IsInMarshaling(long)
@ stdcall TF_MlngInfoCount()
@ stdcall TF_RunInputCPL()
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall SetInputScope(long long)
@ stdcall SetInputScopeXML(ptr ptr)
@ stdcall SetInputScopes(long ptr long ptr long wstr wstr)
@ stdcall TF_CUASAppFix(str)
@ stdcall TF_CheckThreadInputIdle(long long)
@ stdcall TF_ClearLangBarAddIns(ptr)
@ stdcall TF_CreateCategoryMgr(ptr)
@ stdcall TF_CreateCicLoadMutex(ptr)
@ stdcall TF_CreateDisplayAttributeMgr(ptr)
@ stdcall TF_CreateInputProcessorProfiles(ptr)
@ stdcall TF_CreateLangBarItemMgr(ptr)
@ stdcall TF_CreateLangBarMgr(ptr)
@ stdcall TF_CreateThreadMgr(ptr)
@ stdcall TF_DllDetachInOther()
@ stdcall TF_GetGlobalCompartment(ptr)
@ stdcall TF_GetInputScope(ptr ptr)
@ stdcall TF_GetThreadMgr(ptr)
@ stdcall TF_InitSystem()
@ stdcall TF_InvalidAssemblyListCache()
@ stdcall TF_InvalidAssemblyListCacheIfExist()
@ stdcall TF_IsCtfmonRunning()
@ stdcall TF_IsFullScreenWindowAcitvated() ; Yes, Microsoft really misspelled this one!
@ stdcall TF_PostAllThreadMsg(ptr long)
@ stdcall TF_RegisterLangBarAddIn(ptr wstr long)
@ stdcall TF_UninitSystem()
@ stdcall TF_UnregisterLangBarAddIn(ptr long)
