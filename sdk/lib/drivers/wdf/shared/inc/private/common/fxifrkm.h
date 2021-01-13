//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __FXIFRKM_H__
#define __FXIFRKM_H__

#define FX_IFR_MAX_BUFFER_SIZE      (0x10000)   // 64k

#if (PAGE_SIZE > 0x1000)
#define FX_IFR_AVG_BUFFER_SIZE      (3 * PAGE_SIZE) // ia64
#else
#define FX_IFR_AVG_BUFFER_SIZE      (5 * PAGE_SIZE) // x86, amd64
#endif

//
// Log Size should be in whole pages.
//
enum FxIFRValues {
    FxIFRMinLogPages    = 1,
    FxIFRMaxLogPages    = FX_IFR_MAX_BUFFER_SIZE/PAGE_SIZE,
    FxIFRAvgLogPages    = FX_IFR_AVG_BUFFER_SIZE/PAGE_SIZE,

    FxIFRMinLogSize     = FxIFRMinLogPages * PAGE_SIZE,
    FxIFRMaxLogSize     = FxIFRMaxLogPages * PAGE_SIZE,
    FxIFRAvgLogSize     = FxIFRAvgLogPages * PAGE_SIZE,

    FxIFRMaxMessageSize = 256,

    FxIFRRecordSignature = WDF_IFR_RECORD_SIGNATURE,
};

//
// Verify the following:
// - max_log_size must be <= 64k.
// - min_log_size must be >= page_size.
// - max_log_size  >= avg_log_size  >= min_log_size.
// - max_log_pages >= avg_log_pages >= min_log_pages.
//
C_ASSERT(FxIFRMaxLogSize <= 0x10000);
C_ASSERT(FxIFRMinLogSize >= PAGE_SIZE);
C_ASSERT(FxIFRMaxLogSize >= FxIFRAvgLogSize &&
            FxIFRAvgLogSize >= FxIFRMinLogSize);
C_ASSERT(FxIFRMaxLogPages >= FxIFRAvgLogPages &&
            FxIFRAvgLogPages >= FxIFRMinLogPages);


__inline
VOID
FxVerifyLogHeader(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_IFR_HEADER Header
    )
/*++

Routine Description:
    This routine is added to track the down IFR header corruption



--*/
{
    WDFCASSERT(WDF_IFR_HEADER_NAME_LEN == WDF_DRIVER_GLOBALS_NAME_LEN);

    if (FxDriverGlobals->FxVerifierOn
        &&
        (strncmp(Header->DriverName, FxDriverGlobals->Public.DriverName,
                 WDF_IFR_HEADER_NAME_LEN) != 0
         ||
         FxIsEqualGuid((LPGUID)&(Header->Guid), (LPGUID)&WdfTraceGuid) == FALSE
         ||
         Header->Base != (PUCHAR) &Header[1]
         ||
         Header->Offset.u.s.Current > Header->Size
         ||
         Header->Offset.u.s.Previous > Header->Size
         ||
         Header->Size >= FxIFRMaxLogSize)) // size doesn't include header.
    {
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
}

#endif // __FXIFRKM_H__
