@ stdcall Chkdsk(wstr wstr long long long long ptr ptr ptr)
@ stub ChkdskEx
@ stub ComputeFmMediaType
@ stdcall DiskCopy()
@ stdcall EnableVolumeCompression(wstr long)
@ stdcall Extend()
@ stdcall Format(wstr ptr wstr wstr long ptr)
@ stdcall FormatEx(wstr ptr wstr wstr long long ptr)
@ stub FormatEx2
@ stdcall QueryAvailableFileSystemFormat(long wstr str str ptr)
@ stdcall QueryDeviceInformation(wstr ptr long)
@ stub QueryDeviceInformationByHandle
@ stub QueryFileSystemName
@ stub QueryLatestFileSystemVersion
@ stdcall QuerySupportedMedia(wstr ptr long ptr)
@ stdcall SetLabel(wstr wstr) kernel32.SetVolumeLabelW
