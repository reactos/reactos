@ stub DeleteServicePlugPlayRegKeys

@ stdcall PNP_GetDeviceList(long ptr ptr ptr long)
@ stdcall PNP_GetDeviceListSize(long ptr ptr long)
@ stdcall PNP_GetDeviceRegProp(long ptr long ptr ptr ptr ptr long)
@ stdcall PNP_HwProfFlags(long long ptr long ptr ptr ptr long long)
@ stdcall PNP_SetActiveService(long ptr long)

@ stub RegisterScmCallback
@ stub RegisterServiceNotification
@ stdcall ServiceMain(long ptr) ;; If using SvcHost.exe (Vista+)
;@ stdcall SvcEntry_PlugPlay(long long long long) ;; If using services.exe (<= 2k3)
