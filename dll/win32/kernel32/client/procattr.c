/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        dll/win32/kernel32/client/procattr.c
 * PURPOSE:     Process/thread attribute list APIs for CreateProcess
 * COPYRIGHT:   Copyright 1996, 1998 Alexandre Julliard
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Attribute-list layout and Initialize/Update/Delete behavior from
 * dll/win32/kernelbase/wine/process.c (Wine, already in this tree).
 * Guest serial output/ntdll-apps/serial-App-20261116T185747Z.log and
 * firefox-mesa-retest-guest-log/firefox-stderr.txt name
 * SB::LA::SpawnTarget Error 0x80070057 after kernel32 -stub exports.
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

struct proc_thread_attr
{
    DWORD_PTR attr;
    SIZE_T size;
    void *value;
};

struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD mask;
    DWORD size;
    DWORD count;
    DWORD pad;
    DWORD_PTR unk;
    struct proc_thread_attr attrs[1];
};

/*
 * @implemented
 */
BOOL
WINAPI
InitializeProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST list,
                                  DWORD count,
                                  DWORD flags,
                                  PSIZE_T size)
{
    SIZE_T needed;
    BOOL ret = FALSE;

    DPRINT("InitializeProcThreadAttributeList(%p %lu %lx %p)\n", list, count, flags, size);

    if (!size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    needed = FIELD_OFFSET(struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[count]);
    if (list && *size >= needed)
    {
        list->mask = 0;
        list->size = count;
        list->count = 0;
        list->pad = 0;
        list->unk = 0;
        ret = TRUE;
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    *size = needed;
    return ret;
}

static DWORD
validate_proc_thread_attribute(DWORD_PTR attr, SIZE_T size)
{
    switch (attr)
    {
    case PROC_THREAD_ATTRIBUTE_PARENT_PROCESS:
        if (size != sizeof(HANDLE)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS:
        if (size != sizeof(ULONG)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_HANDLE_LIST:
        if ((size / sizeof(HANDLE)) * sizeof(HANDLE) != size) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR:
        if (size != sizeof(PROCESSOR_NUMBER)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_CHILD_PROCESS_POLICY:
        if (size != sizeof(DWORD) && size != sizeof(DWORD64)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY:
        if (size != sizeof(DWORD) && size != sizeof(DWORD64) && size != sizeof(DWORD64) * 2)
            return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE:
        /* HPCON is an opaque handle; size matches a pointer. */
        if (size != sizeof(void *)) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_JOB_LIST:
        if ((size / sizeof(HANDLE)) * sizeof(HANDLE) != size) return ERROR_BAD_LENGTH;
        break;
    case PROC_THREAD_ATTRIBUTE_MACHINE_TYPE:
        if (size != sizeof(USHORT)) return ERROR_BAD_LENGTH;
        break;
    default:
        /*
         * CreateProcessInternalW does not apply the list yet (flag stripped).
         * Still accept unknown attributes so Firefox socket/rdd SpawnTarget
         * can call CreateProcess. Guest:
         * output/ntdll-apps/firefox-procattr-guest-log/firefox-stderr.txt
         * still named socket SpawnTarget 0x80070057 after content children
         * started.
         */
        DPRINT1("Ignoring ProcThreadAttribute %Iu\n", attr & PROC_THREAD_ATTRIBUTE_NUMBER);
        return 0;
    }
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
UpdateProcThreadAttribute(LPPROC_THREAD_ATTRIBUTE_LIST list,
                          DWORD flags,
                          DWORD_PTR attr,
                          PVOID value,
                          SIZE_T size,
                          PVOID prev_ret,
                          PSIZE_T size_ret)
{
    DWORD mask, err;
    struct proc_thread_attr *entry;

    DPRINT("UpdateProcThreadAttribute(%p %lx %08Ix %p %Id %p %p)\n",
           list, flags, attr, value, size, prev_ret, size_ret);

    if (!list)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (list->count >= list->size)
    {
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }

    if ((err = validate_proc_thread_attribute(attr, size)))
    {
        SetLastError(err);
        return FALSE;
    }

    mask = 1u << (attr & PROC_THREAD_ATTRIBUTE_NUMBER);
    if (list->mask & mask)
    {
        SetLastError(ERROR_OBJECT_NAME_EXISTS);
        return FALSE;
    }
    list->mask |= mask;

    entry = list->attrs + list->count;
    entry->attr = attr;
    entry->size = size;
    entry->value = value;
    list->count++;
    return TRUE;
}

/*
 * @implemented
 */
VOID
WINAPI
DeleteProcThreadAttributeList(LPPROC_THREAD_ATTRIBUTE_LIST list)
{
    UNREFERENCED_PARAMETER(list);
}
