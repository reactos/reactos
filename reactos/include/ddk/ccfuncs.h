VOID CbInitDccb(PDCCB Dccb, PDEVICE_OBJECT DeviceObject, ULONG SectorSize,
		ULONG NrSectors, ULONG PercentageToCache);
PCCB CbAcquireForRead(PDCCB Dccb, ULONG BlockNr);
VOID CbReleaseFromRead(PDCCB Dccb, PCCB Ccb);
