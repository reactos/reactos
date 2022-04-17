@ stdcall LsaApCallPackage(ptr ptr ptr long ptr ptr ptr)
@ stdcall LsaApCallPackagePassthrough(ptr ptr ptr long ptr ptr ptr)
@ stdcall LsaApCallPackageUntrusted(ptr ptr ptr long ptr ptr ptr)
@ stdcall LsaApInitializePackage(long ptr ptr ptr ptr)
@ stdcall LsaApLogonTerminated(ptr)
@ stdcall LsaApLogonUserEx2(ptr long ptr ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub Msv1_0ExportSubAuthenticationRoutine
@ stub Msv1_0SubAuthenticationPresent
@ stub MsvGetLogonAttemptCount
@ stub MsvSamLogoff
@ stub MsvSamValidate
@ stub MsvValidateTarget
@ stdcall SpInitialize(long ptr ptr)
@ stdcall SpInstanceInit(long ptr ptr)
@ stdcall SpLsaModeInitialize(long ptr ptr ptr)
@ stdcall SpUserModeInitialize(long ptr ptr ptr)
