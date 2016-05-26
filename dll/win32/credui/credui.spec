@ stub CredUICmdLinePromptForCredentialsA
@ stub CredUICmdLinePromptForCredentialsW
@ stub CredUIConfirmCredentialsA
@ stdcall CredUIConfirmCredentialsW(wstr long)
@ stdcall CredUIInitControls()
@ stub CredUIParseUserNameA
@ stdcall CredUIParseUserNameW(wstr ptr long ptr long)
@ stub CredUIPromptForCredentialsA
@ stdcall CredUIPromptForCredentialsW(ptr wstr ptr long ptr long ptr long ptr long)
@ stdcall CredUIReadSSOCredA(str ptr)
@ stdcall CredUIReadSSOCredW(wstr ptr)
@ stdcall CredUIStoreSSOCredA(str str str long)
@ stdcall CredUIStoreSSOCredW(wstr wstr wstr long)
@ stub -private DllCanUnloadNow
@ stub -private DllGetClassObject
@ stub -private DllRegisterServer
@ stub -private DllUnregisterServer
@ stdcall SspiPromptForCredentialsW(wstr ptr long wstr ptr ptr ptr long)
