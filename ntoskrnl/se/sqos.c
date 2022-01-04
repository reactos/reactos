/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security Quality of Service (SQoS) implementation support
 * COPYRIGHT:       Copyright David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *******************************************************************/

/**
 * @brief
 * Captures the security quality of service data given the object
 * attributes from an object.
 *
 * @param[in] ObjectAttributes
 * Attributes of an object where SQOS is to be retrieved. If the caller
 * doesn't fill object attributes to the function, it automatically assumes
 * SQOS is not present, or if, there's no SQOS present in the object attributes list
 * of the object itself.
 *
 * @param[in] AccessMode
 * Processor access mode.
 *
 * @param[in] PoolType
 * The pool type for the captured SQOS to be used for allocation.
 *
 * @param[in] CaptureIfKernel
 * Capture access condition. To be set to TRUE if the capture is done within the kernel,
 * FALSE if the capture is done in a kernel mode driver or user mode otherwise.
 *
 * @param[out] CapturedSecurityQualityOfService
 * The captured SQOS data from the object.
 *
 * @param[out] Present
 * Returns TRUE if SQOS is present in an object, FALSE otherwise. FALSE is also immediately
 * returned if no object attributes is given to the call.
 *
 * @return
 * STATUS_SUCCESS if SQOS from the object has been fully and successfully captured. STATUS_INVALID_PARAMETER
 * if the caller submits an invalid object attributes list. STATUS_INSUFFICIENT_RESOURCES if the function has
 * failed to allocate some resources in the pool for the captured SQOS. A failure NTSTATUS code is returned
 * otherwise.
 */
NTSTATUS
NTAPI
SepCaptureSecurityQualityOfService(
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSECURITY_QUALITY_OF_SERVICE *CapturedSecurityQualityOfService,
    _Out_ PBOOLEAN Present)
{
    PSECURITY_QUALITY_OF_SERVICE CapturedQos;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(CapturedSecurityQualityOfService);
    ASSERT(Present);

    if (ObjectAttributes != NULL)
    {
        if (AccessMode != KernelMode)
        {
            SECURITY_QUALITY_OF_SERVICE SafeQos;

            _SEH2_TRY
            {
                ProbeForRead(ObjectAttributes,
                             sizeof(OBJECT_ATTRIBUTES),
                             sizeof(ULONG));
                if (ObjectAttributes->Length == sizeof(OBJECT_ATTRIBUTES))
                {
                    if (ObjectAttributes->SecurityQualityOfService != NULL)
                    {
                        ProbeForRead(ObjectAttributes->SecurityQualityOfService,
                                     sizeof(SECURITY_QUALITY_OF_SERVICE),
                                     sizeof(ULONG));

                        if (((PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService)->Length ==
                            sizeof(SECURITY_QUALITY_OF_SERVICE))
                        {
                            /*
                             * Don't allocate memory here because ExAllocate should bugcheck
                             * the system if it's buggy, SEH would catch that! So make a local
                             * copy of the qos structure.
                             */
                            RtlCopyMemory(&SafeQos,
                                          ObjectAttributes->SecurityQualityOfService,
                                          sizeof(SECURITY_QUALITY_OF_SERVICE));
                            *Present = TRUE;
                        }
                        else
                        {
                            Status = STATUS_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        *CapturedSecurityQualityOfService = NULL;
                        *Present = FALSE;
                    }
                }
                else
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            if (NT_SUCCESS(Status))
            {
                if (*Present)
                {
                    CapturedQos = ExAllocatePoolWithTag(PoolType,
                                                        sizeof(SECURITY_QUALITY_OF_SERVICE),
                                                        TAG_QOS);
                    if (CapturedQos != NULL)
                    {
                        RtlCopyMemory(CapturedQos,
                                      &SafeQos,
                                      sizeof(SECURITY_QUALITY_OF_SERVICE));
                        *CapturedSecurityQualityOfService = CapturedQos;
                    }
                    else
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    *CapturedSecurityQualityOfService = NULL;
                }
            }
        }
        else
        {
            if (ObjectAttributes->Length == sizeof(OBJECT_ATTRIBUTES))
            {
                if (CaptureIfKernel)
                {
                    if (ObjectAttributes->SecurityQualityOfService != NULL)
                    {
                        if (((PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService)->Length ==
                            sizeof(SECURITY_QUALITY_OF_SERVICE))
                        {
                            CapturedQos = ExAllocatePoolWithTag(PoolType,
                                                                sizeof(SECURITY_QUALITY_OF_SERVICE),
                                                                TAG_QOS);
                            if (CapturedQos != NULL)
                            {
                                RtlCopyMemory(CapturedQos,
                                              ObjectAttributes->SecurityQualityOfService,
                                              sizeof(SECURITY_QUALITY_OF_SERVICE));
                                *CapturedSecurityQualityOfService = CapturedQos;
                                *Present = TRUE;
                            }
                            else
                            {
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                        else
                        {
                            Status = STATUS_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        *CapturedSecurityQualityOfService = NULL;
                        *Present = FALSE;
                    }
                }
                else
                {
                    *CapturedSecurityQualityOfService = (PSECURITY_QUALITY_OF_SERVICE)ObjectAttributes->SecurityQualityOfService;
                    *Present = (ObjectAttributes->SecurityQualityOfService != NULL);
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        *CapturedSecurityQualityOfService = NULL;
        *Present = FALSE;
    }

    return Status;
}

/**
 * @brief
 * Releases (frees) the captured SQOS data from an object in the memory pool.
 *
 * @param[in] CapturedSecurityQualityOfService
 * The captured SQOS data to be released.
 *
 * @param[in] AccessMode
 * Processor access mode.
 *
 * @param[in] CaptureIfKernel
 * Capture access condition. To be set to TRUE if the capture is done within the kernel,
 * FALSE if the capture is done in a kernel mode driver or user mode otherwise.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepReleaseSecurityQualityOfService(
    _In_opt_ PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel)
{
    PAGED_CODE();

    if (CapturedSecurityQualityOfService != NULL &&
        (AccessMode != KernelMode || CaptureIfKernel))
    {
        ExFreePoolWithTag(CapturedSecurityQualityOfService, TAG_QOS);
    }
}

/* EOF */
