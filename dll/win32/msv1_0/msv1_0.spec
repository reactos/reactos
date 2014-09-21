@ stdcall LsaApCallPackage(ptr ptr ptr long ptr ptr ptr)
@ stdcall LsaApCallPackagePassthrough(ptr ptr ptr long ptr ptr ptr)
@ stdcall LsaApCallPackageUntrusted(ptr ptr ptr long ptr ptr ptr)
@ stdcall LsaApInitializePackage(long ptr ptr ptr ptr)
@ stdcall LsaApLogonTerminated(ptr)
@ stdcall LsaApLogonUser(ptr long ptr ptr long ptr ptr ptr ptr ptr ptr ptr ptr)
#@ stdcall LsaApLogonUserEx(ptr long ptr ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr)
#@ stdcall LsaApLogonUserEx2(ptr long ptr ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub Msv1_0ExportSubAuthenticationRoutine
@ stub Msv1_0SubAuthenticationPresent
@ stub MsvGetLogonAttemptCount
@ stub MsvSamLogoff
@ stub MsvSamValidate
@ stub MsvValidateTarget
@ stub SpInitialize
@ stub SpInstanceInit
@ stub SpLsaModeInitiaize
@ stub SpUserModeInitiaize
