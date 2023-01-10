enum NHLTQueryRevision {
	NHLTRev1 = 1
};

enum NHLTQuery {
	NHLTSupportQuery = 0,
	NHLTMemoryAddress
};

NTSTATUS NHLTCheckSupported(_In_ WDFDEVICE FxDevice);
NTSTATUS NHLTQueryTableAddress(_In_ WDFDEVICE FxDevice, UINT64* nhltAddr, UINT64* nhltSz);