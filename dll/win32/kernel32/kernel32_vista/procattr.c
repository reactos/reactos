/*
 * PROJECT:     ReactOS system libraries
 * FILE:        dll/win32/kernel32/kernel32_vista/procattr.c
 * PURPOSE:     Process and thread attribute lists
 * PROGRAMMERS: Mohammed Al-Houmi <mhmdlylyalhwmy461@gmail.com>
 *
 * NOTE:        Layout of the attribute list is not documented anywhere,
 *              but the one used here is stable and matches what Wine does.
 *              kernel32's CreateProcess doesn't read the list back yet,
 *              so for now all we do is store what the app tells us.
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

typedef struct _K32_PROC_THREAD_ATTRIBUTE
{
    DWORD_PTR Attribute;
    SIZE_T cbSize;
    PVOID lpValue;
} K32_PROC_THREAD_ATTRIBUTE;

struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD_PTR dwMask;       /* bitmask of attributes currently stored */
    DWORD dwAttributes;     /* capacity, set by Initialize... */
    DWORD dwCount;          /* how many attributes we have stored */
    DWORD_PTR dwReserved;   /* keep the array aligned */
    K32_PROC_THREAD_ATTRIBUTE Attributes[1];
};

/*** The attributes below exist on Vista and up. For anything we don't
     know about, just fail instead of silently storing bogus data. ***/
static DWORD
ProcThreadAttributeIsValid(_In_ DWORD_PTR dwAttribute, _In_ SIZE_T cbSize)
{
    DWORD_PTR dwNumber = dwAttribute & PROC_THREAD_ATTRIBUTE_NUMBER;

    switch (dwNumber)
    {
        case (ProcThreadAttributeParentProcess):
            if (cbSize != sizeof(HANDLE)) return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributeExtendedFlags):
            if (cbSize != sizeof(ULONG)) return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributeHandleList):
        case (ProcThreadAttributeJobList):
            /* must be an exact multiple of HANDLE */
            if ((cbSize / sizeof(HANDLE)) * sizeof(HANDLE) != cbSize)
                return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributeIdealProcessor):
            if (cbSize != sizeof(PROCESSOR_NUMBER)) return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributeMitigationPolicy):
            if (cbSize != sizeof(DWORD) && cbSize != sizeof(DWORD64) &&
                cbSize != sizeof(DWORD64) * 2)
                return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributeChildProcessPolicy):
            if (cbSize != sizeof(DWORD) && cbSize != sizeof(DWORD64))
                return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributePseudoConsole):
            /* HPCON is a pointer-sized handle, we can't use the type here */
            if (cbSize != sizeof(void *)) return ERROR_BAD_LENGTH;
            break;
        case (ProcThreadAttributeMachineType):
            if (cbSize != sizeof(USHORT)) return ERROR_BAD_LENGTH;
            break;
        default:
            FIXME("Unhandled attribute number %Ix\n", dwNumber);
            return ERROR_NOT_SUPPORTED;
    }

    return ERROR_SUCCESS;
}

/*
 * Get the required size of the list, or fill the caller's buffer.
 * When lpAttributeList is NULL we are being asked for the size, which
 * fails with ERROR_INSUFFICIENT_BUFFER (and sets *lpSize) just like on
 * real Windows, so apps know how much to allocate.
 */
BOOL
WINAPI
InitializeProcThreadAttributeList(_Inout_opt_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
                                  _In_ DWORD dwAttributeCount,
                                  _In_ DWORD dwFlags,
                                  _Inout_ PSIZE_T lpSize)
{
    struct _PROC_THREAD_ATTRIBUTE_LIST *AttributeList = lpAttributeList;
    SIZE_T RequiredSize;

    TRACE("(%p, %lu, %lx, %p)\n", lpAttributeList, dwAttributeCount, dwFlags, lpSize);

    (void)dwFlags;

    if (!lpSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RequiredSize = FIELD_OFFSET(struct _PROC_THREAD_ATTRIBUTE_LIST,
                                Attributes[dwAttributeCount]);

    if (AttributeList && *lpSize >= RequiredSize)
    {
        AttributeList->dwMask = 0;
        AttributeList->dwAttributes = dwAttributeCount;
        AttributeList->dwCount = 0;
        AttributeList->dwReserved = 0;
        *lpSize = RequiredSize;
        return TRUE;
    }

    *lpSize = RequiredSize;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

BOOL
WINAPI
UpdateProcThreadAttribute(_In_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
                          _In_ DWORD dwFlags,
                          _In_ DWORD_PTR Attribute,
                          _In_ PVOID lpValue,
                          _In_ SIZE_T cbSize,
                          _Out_opt_ PVOID lpPreviousValue,
                          _Out_opt_ PSIZE_T lpReturnSize)
{
    struct _PROC_THREAD_ATTRIBUTE_LIST *AttributeList = lpAttributeList;
    K32_PROC_THREAD_ATTRIBUTE *Entry;
    DWORD_PTR Mask;
    DWORD dwError;

    TRACE("(%p, %lx, %Ix, %p, %Iu, %p, %p)\n", lpAttributeList, dwFlags,
          Attribute, lpValue, cbSize, lpPreviousValue, lpReturnSize);

    /* We don't report previous values yet. Callers that need them will
     * have to wait, but nobody in-tree uses that. */
    (void)dwFlags;
    (void)lpPreviousValue;
    (void)lpReturnSize;

    dwError = ProcThreadAttributeIsValid(Attribute, cbSize);
    if (dwError != ERROR_SUCCESS)
    {
        SetLastError(dwError);
        return FALSE;
    }

    Mask = (DWORD_PTR)1 << (Attribute & PROC_THREAD_ATTRIBUTE_NUMBER);
    if (AttributeList->dwMask & Mask)
    {
        SetLastError(ERROR_OBJECT_NAME_EXISTS);
        return FALSE;
    }

    if (AttributeList->dwCount >= AttributeList->dwAttributes)
    {
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }

    /* store a copy of the caller's values */
    Entry = &AttributeList->Attributes[AttributeList->dwCount];
    Entry->Attribute = Attribute;
    Entry->cbSize = cbSize;
    Entry->lpValue = lpValue;

    AttributeList->dwMask |= Mask;
    AttributeList->dwCount++;

    return TRUE;
}

VOID
WINAPI
DeleteProcThreadAttributeList(_In_opt_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList)
{
    /* nothing stored in the list is owned by us, so nothing to free.
     * add a trace in case the list is ever consumed by CreateProcess. */
    TRACE("(%p)\n", lpAttributeList);
}
