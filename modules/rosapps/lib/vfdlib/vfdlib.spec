@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)

@ stdcall VfdRegisterHandlers()
@ stdcall VfdUnregisterHandlers()
@ stdcall VfdCheckHandlers()

@ stdcall VfdInstallDriver(str long)
@ stdcall VfdConfigDriver(long)
@ stdcall VfdRemoveDriver()
@ stdcall VfdStartDriver(ptr)
@ stdcall VfdStopDriver(ptr)
@ stdcall VfdGetDriverConfig(str ptr)
@ stdcall VfdGetDriverState(ptr)

@ stdcall VfdOpenDevice(long)
@ stdcall VfdGetDeviceNumber(ptr ptr)
@ stdcall VfdGetDeviceName(ptr str long)
@ stdcall VfdGetDriverVersion(ptr ptr)

@ stdcall VfdOpenImage(ptr str long long long)
@ stdcall VfdCloseImage(ptr long)
@ stdcall VfdGetImageInfo(ptr str ptr ptr ptr ptr ptr)
@ stdcall VfdSaveImage(ptr str long long)
@ stdcall VfdFormatMedia(ptr)
@ stdcall VfdGetMediaState(ptr)
@ stdcall VfdWriteProtect(ptr long)
@ stdcall VfdDismountVolume(ptr long)

@ stdcall VfdSetGlobalLink(ptr long)
@ stdcall VfdGetGlobalLink(ptr str)
@ stdcall VfdSetLocalLink(ptr long)
@ stdcall VfdGetLocalLink(ptr str)

@ stdcall VfdGetNotifyMessage()

@ stdcall VfdChooseLetter()
@ stdcall VfdCheckDriverFile(str ptr)
@ stdcall VfdCheckImageFile(str ptr ptr ptr)
@ stdcall VfdCreateImageFile(str long long long)

@ stdcall VfdLookupMedia(long)
@ stdcall VfdGetMediaSize(long)
@ stdcall VfdMediaTypeName(long)

@ stdcall VfdMakeFileDesc(str long long long long)

@ stdcall VfdGuiOpen(ptr long)
@ stdcall VfdGuiSave(ptr long)
@ stdcall VfdGuiClose(ptr long)
@ stdcall VfdGuiFormat(ptr long)
@ stdcall VfdToolTip(ptr str long long long)
@ stdcall VfdImageTip(ptr long)

@ stdcall VfdIsValidPlatform()
