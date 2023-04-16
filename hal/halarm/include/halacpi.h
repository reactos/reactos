#pragma once

NTSTATUS
NTAPI
HalpAcpiTableCacheInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

PVOID
NTAPI
HalpAcpiGetTable(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Signature
);

CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpSetupAcpiPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

PVOID
NTAPI
HalAcpiGetTable(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Signature
);

/* EOF */
