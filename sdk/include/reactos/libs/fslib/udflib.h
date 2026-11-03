/* Native UDF 2.50 formatting provider. */
#pragma once
#include <fmifs/fmifs.h>
BOOLEAN NTAPI UdfFormat(PUNICODE_STRING DriveRoot, PFMIFSCALLBACK Callback,
                       BOOLEAN QuickFormat, BOOLEAN BackwardCompatible,
                       MEDIA_TYPE MediaType, PUNICODE_STRING Label, ULONG ClusterSize);

/* Checks a UDF 2.50 volume with the UDFS recovery core and, with
 * FixErrors, repairs what that core can repair safely. */
BOOLEAN NTAPI UdfChkdsk(PUNICODE_STRING DriveRoot, PFMIFSCALLBACK Callback,
                        BOOLEAN FixErrors, BOOLEAN Verbose, BOOLEAN CheckOnlyIfDirty,
                        BOOLEAN ScanDrive, PVOID pUnknown1, PVOID pUnknown2,
                        PVOID pUnknown3, PVOID pUnknown4, PULONG ExitStatus);
