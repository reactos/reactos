typedef struct
{
	LPSTR LeftVolumeName;
	LPSTR RightVolumeName;
	ULONG DefaultVolume;
	ULONG Type;
	ULONG DeviceType;
	char Key[4];
	LPSTR PrototypeName;
	PVOID DeferredRoutine;
	PVOID ExclusionRoutine;
	PVOID DispatchRoutine;
	PVOID DevCapsRoutine;
	PVOID HwSetVolume;
	ULONG IoMethod;
}SOUND_DEVICE_INIT;