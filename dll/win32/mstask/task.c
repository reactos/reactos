/*
 * Copyright (C) 2008 Google (Roy Shea)
 * Copyright (C) 2018 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "objbase.h"
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

typedef struct
{
    USHORT product_version;
    USHORT file_version;
    UUID uuid;
    USHORT name_size_offset;
    USHORT trigger_offset;
    USHORT error_retry_count;
    USHORT error_retry_interval;
    USHORT idle_deadline;
    USHORT idle_wait;
    UINT priority;
    UINT maximum_runtime;
    UINT exit_code;
    HRESULT status;
    UINT flags;
    SYSTEMTIME last_runtime;
} FIXDLEN_DATA;

typedef struct
{
    ITask ITask_iface;
    IPersistFile IPersistFile_iface;
    LONG ref;
    ITaskDefinition *task;
    IExecAction *action;
    BYTE *data;
    WORD data_count;
    UUID uuid;
    LPWSTR task_name;
    HRESULT status;
    WORD idle_minutes, deadline_minutes;
    DWORD flags, priority, maxRunTime, exit_code;
    SYSTEMTIME last_runtime;
    LPWSTR accountName;
    DWORD trigger_count;
    TASK_TRIGGER *trigger;
    BOOL is_dirty;
    USHORT instance_count;
} TaskImpl;

static inline TaskImpl *impl_from_ITask(ITask *iface)
{
    return CONTAINING_RECORD(iface, TaskImpl, ITask_iface);
}

static inline TaskImpl *impl_from_IPersistFile( IPersistFile *iface )
{
    return CONTAINING_RECORD(iface, TaskImpl, IPersistFile_iface);
}

static void TaskDestructor(TaskImpl *This)
{
    TRACE("%p\n", This);
    if (This->action)
        IExecAction_Release(This->action);
    ITaskDefinition_Release(This->task);
    free(This->data);
    free(This->task_name);
    free(This->accountName);
    free(This->trigger);
    free(This);
    InterlockedDecrement(&dll_ref);
}

static HRESULT WINAPI MSTASK_ITask_QueryInterface(
        ITask* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskImpl * This = impl_from_ITask(iface);

    TRACE("IID: %s\n", debugstr_guid(riid));
    if (ppvObject == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITask))
    {
        *ppvObject = &This->ITask_iface;
        ITask_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IPersistFile))
    {
        *ppvObject = &This->IPersistFile_iface;
        ITask_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITask_AddRef(
        ITask* iface)
{
    TaskImpl *This = impl_from_ITask(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_ITask_Release(
        ITask* iface)
{
    TaskImpl * This = impl_from_ITask(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskDestructor(This);
    return ref;
}

HRESULT task_set_trigger(ITask *task, WORD idx, const TASK_TRIGGER *src)
{
    TaskImpl *This = impl_from_ITask(task);
    TIME_FIELDS field_time;
    LARGE_INTEGER sys_time;
    TASK_TRIGGER dst;

    TRACE("(%p, %u, %p)\n", task, idx, src);

    if (idx >= This->trigger_count)
        return E_FAIL;

    /* Verify valid structure size */
    if (src->cbTriggerSize != sizeof(*src))
        return E_INVALIDARG;
    dst.cbTriggerSize = src->cbTriggerSize;

    /* Reserved field must be zero */
    dst.Reserved1 = 0;

    /* Verify and set valid start date and time */
    memset(&field_time, 0, sizeof(field_time));
    field_time.Year = src->wBeginYear;
    field_time.Month = src->wBeginMonth;
    field_time.Day = src->wBeginDay;
    field_time.Hour = src->wStartHour;
    field_time.Minute = src->wStartMinute;
    if (!RtlTimeFieldsToTime(&field_time, &sys_time))
        return E_INVALIDARG;
    dst.wBeginYear = src->wBeginYear;
    dst.wBeginMonth = src->wBeginMonth;
    dst.wBeginDay = src->wBeginDay;
    dst.wStartHour = src->wStartHour;
    dst.wStartMinute = src->wStartMinute;

    /* Verify valid end date if TASK_TRIGGER_FLAG_HAS_END_DATE flag is set */
    if (src->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
    {
        memset(&field_time, 0, sizeof(field_time));
        field_time.Year = src->wEndYear;
        field_time.Month = src->wEndMonth;
        field_time.Day = src->wEndDay;
        if (!RtlTimeFieldsToTime(&field_time, &sys_time))
            return E_INVALIDARG;
    }

    /* Set valid end date independent of TASK_TRIGGER_FLAG_HAS_END_DATE flag */
    dst.wEndYear = src->wEndYear;
    dst.wEndMonth = src->wEndMonth;
    dst.wEndDay = src->wEndDay;

    /* Verify duration and interval pair */
    if (src->MinutesDuration <= src->MinutesInterval && src->MinutesInterval > 0)
        return E_INVALIDARG;
    dst.MinutesDuration = src->MinutesDuration;
    dst.MinutesInterval = src->MinutesInterval;

    /* Copy over flags */
    dst.rgFlags = src->rgFlags;

    /* Set TriggerType dependent fields of Type union */
    dst.TriggerType = src->TriggerType;
    switch (src->TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
            dst.Type.Daily.DaysInterval = src->Type.Daily.DaysInterval;
            break;
        case TASK_TIME_TRIGGER_WEEKLY:
            dst.Type.Weekly.WeeksInterval = src->Type.Weekly.WeeksInterval;
            dst.Type.Weekly.rgfDaysOfTheWeek = src->Type.Weekly.rgfDaysOfTheWeek;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDATE:
            dst.Type.MonthlyDate.rgfDays = src->Type.MonthlyDate.rgfDays;
            dst.Type.MonthlyDate.rgfMonths = src->Type.MonthlyDate.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDOW:
            dst.Type.MonthlyDOW.wWhichWeek = src->Type.MonthlyDOW.wWhichWeek;
            dst.Type.MonthlyDOW.rgfDaysOfTheWeek = src->Type.MonthlyDOW.rgfDaysOfTheWeek;
            dst.Type.MonthlyDOW.rgfMonths = src->Type.MonthlyDOW.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_ONCE:
        case TASK_EVENT_TRIGGER_ON_IDLE:
        case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        case TASK_EVENT_TRIGGER_AT_LOGON:
        default:
            dst.Type = src->Type;
            break;
    }

    /* Reserved field must be zero */
    dst.Reserved2 = 0;

    /* wRandomMinutesInterval not currently used and is initialized to zero */
    dst.wRandomMinutesInterval = 0;

    This->trigger[idx] = dst;

    return S_OK;
}

HRESULT task_get_trigger(ITask *task, WORD idx, TASK_TRIGGER *dst)
{
    TaskImpl *This = impl_from_ITask(task);
    TASK_TRIGGER *src;

    TRACE("(%p, %u, %p)\n", task, idx, dst);

    if (idx >= This->trigger_count)
        return SCHED_E_TRIGGER_NOT_FOUND;

    src = &This->trigger[idx];

    /* Native implementation doesn't verify equivalent cbTriggerSize fields */

    /* Copy relevant fields of the structure */
    dst->cbTriggerSize = src->cbTriggerSize;
    dst->Reserved1 = 0;
    dst->wBeginYear = src->wBeginYear;
    dst->wBeginMonth = src->wBeginMonth;
    dst->wBeginDay = src->wBeginDay;
    dst->wEndYear = src->wEndYear;
    dst->wEndMonth = src->wEndMonth;
    dst->wEndDay = src->wEndDay;
    dst->wStartHour = src->wStartHour;
    dst->wStartMinute = src->wStartMinute;
    dst->MinutesDuration = src->MinutesDuration;
    dst->MinutesInterval = src->MinutesInterval;
    dst->rgFlags = src->rgFlags;
    dst->TriggerType = src->TriggerType;
    switch (src->TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
            dst->Type.Daily.DaysInterval = src->Type.Daily.DaysInterval;
            break;
        case TASK_TIME_TRIGGER_WEEKLY:
            dst->Type.Weekly.WeeksInterval = src->Type.Weekly.WeeksInterval;
            dst->Type.Weekly.rgfDaysOfTheWeek = src->Type.Weekly.rgfDaysOfTheWeek;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDATE:
            dst->Type.MonthlyDate.rgfDays = src->Type.MonthlyDate.rgfDays;
            dst->Type.MonthlyDate.rgfMonths = src->Type.MonthlyDate.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDOW:
            dst->Type.MonthlyDOW.wWhichWeek = src->Type.MonthlyDOW.wWhichWeek;
            dst->Type.MonthlyDOW.rgfDaysOfTheWeek = src->Type.MonthlyDOW.rgfDaysOfTheWeek;
            dst->Type.MonthlyDOW.rgfMonths = src->Type.MonthlyDOW.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_ONCE:
        case TASK_EVENT_TRIGGER_ON_IDLE:
        case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        case TASK_EVENT_TRIGGER_AT_LOGON:
        default:
            break;
    }
    dst->Reserved2 = 0;
    dst->wRandomMinutesInterval = 0;

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_CreateTrigger(ITask *iface, WORD *idx, ITaskTrigger **task_trigger)
{
    TaskImpl *This = impl_from_ITask(iface);
    TASK_TRIGGER *new_trigger;
    SYSTEMTIME time;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, idx, task_trigger);

    hr = TaskTriggerConstructor(iface, This->trigger_count, task_trigger);
    if (hr != S_OK) return hr;

    if (This->trigger)
        new_trigger = realloc(This->trigger, sizeof(This->trigger[0]) * (This->trigger_count + 1));
    else
        new_trigger = malloc(sizeof(This->trigger[0]));
    if (!new_trigger)
    {
        ITaskTrigger_Release(*task_trigger);
        return E_OUTOFMEMORY;
    }

    This->trigger = new_trigger;

    new_trigger = &This->trigger[This->trigger_count];

    /* Most fields default to zero. Initialize other fields to default values. */
    memset(new_trigger, 0, sizeof(*new_trigger));
    GetLocalTime(&time);
    new_trigger->cbTriggerSize = sizeof(*new_trigger);
    new_trigger->wBeginYear = time.wYear;
    new_trigger->wBeginMonth = time.wMonth;
    new_trigger->wBeginDay = time.wDay;
    new_trigger->wStartHour = time.wHour;
    new_trigger->wStartMinute = time.wMinute;
    new_trigger->rgFlags = TASK_TRIGGER_FLAG_DISABLED;
    new_trigger->TriggerType = TASK_TIME_TRIGGER_DAILY;
    new_trigger->Type.Daily.DaysInterval = 1;

    *idx = This->trigger_count++;

    return hr;
}

static HRESULT WINAPI MSTASK_ITask_DeleteTrigger(ITask *iface, WORD idx)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %u)\n", iface, idx);

    if (idx >= This->trigger_count)
        return SCHED_E_TRIGGER_NOT_FOUND;

    This->trigger_count--;
    memmove(&This->trigger[idx], &This->trigger[idx + 1], (This->trigger_count - idx) * sizeof(This->trigger[0]));
    /* this shouldn't fail in practice since we're shrinking the memory block */
    This->trigger = realloc(This->trigger, sizeof(This->trigger[0]) * This->trigger_count);

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetTriggerCount(ITask *iface, WORD *count)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, count);

    *count = This->trigger_count;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetTrigger(ITask *iface, WORD idx, ITaskTrigger **trigger)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %u, %p)\n", iface, idx, trigger);

    if (idx >= This->trigger_count)
        return SCHED_E_TRIGGER_NOT_FOUND;

    return TaskTriggerConstructor(iface, idx, trigger);
}

static HRESULT WINAPI MSTASK_ITask_GetTriggerString(
        ITask* iface,
        WORD iTrigger,
        LPWSTR *ppwszTrigger)
{
    FIXME("(%p, %d, %p): stub\n", iface, iTrigger, ppwszTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetRunTimes(
        ITask* iface,
        const LPSYSTEMTIME pstBegin,
        const LPSYSTEMTIME pstEnd,
        WORD *pCount,
        LPSYSTEMTIME *rgstTaskTimes)
{
    FIXME("(%p, %p, %p, %p, %p): stub\n", iface, pstBegin, pstEnd, pCount,
            rgstTaskTimes);
    return E_NOTIMPL;
}

static void get_begin_time(const TASK_TRIGGER *trigger, FILETIME *ft)
{
    SYSTEMTIME st;

    st.wYear = trigger->wBeginYear;
    st.wMonth = trigger->wBeginMonth;
    st.wDay = trigger->wBeginDay;
    st.wDayOfWeek = 0;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st, ft);
}

static void get_end_time(const TASK_TRIGGER *trigger, FILETIME *ft)
{
    SYSTEMTIME st;

    if (!(trigger->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE))
    {
        ft->dwHighDateTime = ~0u;
        ft->dwLowDateTime = ~0u;
        return;
    }

    st.wYear = trigger->wEndYear;
    st.wMonth = trigger->wEndMonth;
    st.wDay = trigger->wEndDay;
    st.wDayOfWeek = 0;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st, ft);
}

static void filetime_add_ms(FILETIME *ft, ULONGLONG ms)
{
    union u_ftll
    {
        FILETIME ft;
        ULONGLONG ll;
    } *ftll = (union u_ftll *)ft;

    ftll->ll += ms * (ULONGLONG)10000;
}

static void filetime_add_hours(FILETIME *ft, ULONG hours)
{
    filetime_add_ms(ft, (ULONGLONG)hours * 60 * 60 * 1000);
}

static void filetime_add_days(FILETIME *ft, ULONG days)
{
    filetime_add_hours(ft, (ULONGLONG)days * 24);
}

static void filetime_add_weeks(FILETIME *ft, ULONG weeks)
{
    filetime_add_days(ft, (ULONGLONG)weeks * 7);
}

static HRESULT WINAPI MSTASK_ITask_GetNextRunTime(ITask *iface, SYSTEMTIME *rt)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr = SCHED_S_TASK_NO_VALID_TRIGGERS;
    SYSTEMTIME st, current_st;
    FILETIME current_ft, trigger_ft, begin_ft, end_ft, best_ft;
    BOOL have_best_time = FALSE;
    DWORD i;

    TRACE("(%p, %p)\n", iface, rt);

    if (This->flags & TASK_FLAG_DISABLED)
    {
        memset(rt, 0, sizeof(*rt));
        return SCHED_S_TASK_DISABLED;
    }

    GetLocalTime(&current_st);
    SystemTimeToFileTime(&current_st, &current_ft);

    for (i = 0; i < This->trigger_count; i++)
    {
        if (!(This->trigger[i].rgFlags & TASK_TRIGGER_FLAG_DISABLED))
        {
            get_begin_time(&This->trigger[i], &begin_ft);
            if (CompareFileTime(&begin_ft, &current_ft) < 0)
                begin_ft = current_ft;

            get_end_time(&This->trigger[i], &end_ft);

            switch (This->trigger[i].TriggerType)
            {
            case TASK_EVENT_TRIGGER_ON_IDLE:
            case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
            case TASK_EVENT_TRIGGER_AT_LOGON:
                hr = SCHED_S_EVENT_TRIGGER;
                break;

            case TASK_TIME_TRIGGER_ONCE:
                st = current_st;
                st.wYear = This->trigger[i].wBeginYear;
                st.wMonth = This->trigger[i].wBeginMonth;
                st.wDay = This->trigger[i].wBeginDay;
                st.wHour = This->trigger[i].wStartHour;
                st.wMinute = This->trigger[i].wStartMinute;
                st.wSecond = 0;
                st.wMilliseconds = 0;
                SystemTimeToFileTime(&st, &trigger_ft);
                if (CompareFileTime(&begin_ft, &trigger_ft) <= 0 && CompareFileTime(&trigger_ft, &end_ft) < 0)
                {
                    if (!have_best_time || CompareFileTime(&trigger_ft, &best_ft) < 0)
                    {
                        best_ft = trigger_ft;
                        have_best_time = TRUE;
                    }
                }
                break;

            case TASK_TIME_TRIGGER_DAILY:
                if (!This->trigger[i].Type.Daily.DaysInterval)
                    break; /* avoid infinite loop */

                st = current_st;
                st.wHour = This->trigger[i].wStartHour;
                st.wMinute = This->trigger[i].wStartMinute;
                st.wSecond = 0;
                st.wMilliseconds = 0;
                SystemTimeToFileTime(&st, &trigger_ft);
                while (CompareFileTime(&trigger_ft, &end_ft) < 0)
                {
                    if (CompareFileTime(&trigger_ft, &begin_ft) >= 0)
                    {
                        if (!have_best_time || CompareFileTime(&trigger_ft, &best_ft) < 0)
                        {
                            best_ft = trigger_ft;
                            have_best_time = TRUE;
                        }
                        break;
                    }

                    filetime_add_days(&trigger_ft, This->trigger[i].Type.Daily.DaysInterval);
                }
                break;

            case TASK_TIME_TRIGGER_WEEKLY:
                if (!This->trigger[i].Type.Weekly.rgfDaysOfTheWeek)
                    break; /* avoid infinite loop */

                st = current_st;
                st.wHour = This->trigger[i].wStartHour;
                st.wMinute = This->trigger[i].wStartMinute;
                st.wSecond = 0;
                st.wMilliseconds = 0;
                SystemTimeToFileTime(&st, &trigger_ft);
                while (CompareFileTime(&trigger_ft, &end_ft) < 0)
                {
                    FileTimeToSystemTime(&trigger_ft, &st);

                    if (CompareFileTime(&trigger_ft, &begin_ft) >= 0)
                    {
                        if (This->trigger[i].Type.Weekly.rgfDaysOfTheWeek & (1 << st.wDayOfWeek))
                        {
                            if (!have_best_time || CompareFileTime(&trigger_ft, &best_ft) < 0)
                            {
                                best_ft = trigger_ft;
                                have_best_time = TRUE;
                            }
                            break;
                        }
                    }

                    if (st.wDayOfWeek == 0 && This->trigger[i].Type.Weekly.WeeksInterval > 1) /* Sunday, goto next week */
                        filetime_add_weeks(&trigger_ft, This->trigger[i].Type.Weekly.WeeksInterval - 1);
                    else /* check next weekday */
                        filetime_add_days(&trigger_ft, 1);
                }
                break;

            default:
                FIXME("trigger type %u is not handled\n", This->trigger[i].TriggerType);
                break;
            }
        }
    }

    if (have_best_time)
    {
        FileTimeToSystemTime(&best_ft, rt);
        return S_OK;
    }

    memset(rt, 0, sizeof(*rt));
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetIdleWait(
        ITask* iface,
        WORD wIdleMinutes,
        WORD wDeadlineMinutes)
{
    FIXME("(%p, %d, %d): stub\n", iface, wIdleMinutes, wDeadlineMinutes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetIdleWait(ITask *iface, WORD *idle_minutes, WORD *deadline_minutes)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p, %p): stub\n", iface, idle_minutes, deadline_minutes);

    *idle_minutes = This->idle_minutes;
    *deadline_minutes = This->deadline_minutes;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_Run(ITask *iface)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p)\n", iface);

    if (This->status == SCHED_S_TASK_NOT_SCHEDULED)
        return SCHED_E_TASK_NOT_READY;

    This->flags |= 0x04000000;
    return IPersistFile_Save(&This->IPersistFile_iface, NULL, FALSE);
}

static HRESULT WINAPI MSTASK_ITask_Terminate(ITask *iface)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p)\n", iface);

    if (!This->instance_count)
        return SCHED_E_TASK_NOT_RUNNING;

    This->flags |= 0x08000000;
    return IPersistFile_Save(&This->IPersistFile_iface, NULL, FALSE);
}

static HRESULT WINAPI MSTASK_ITask_EditWorkItem(
        ITask* iface,
        HWND hParent,
        DWORD dwReserved)
{
    FIXME("(%p, %p, %ld): stub\n", iface, hParent, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetMostRecentRunTime(ITask *iface, SYSTEMTIME *st)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, st);

    if (This->status == SCHED_S_TASK_NOT_SCHEDULED)
    {
        memset(st, 0, sizeof(*st));
        return SCHED_S_TASK_HAS_NOT_RUN;
    }

    *st = This->last_runtime;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetStatus(ITask *iface, HRESULT *status)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, status);

    *status = This->instance_count ? SCHED_S_TASK_RUNNING : This->status;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetExitCode(ITask *iface, DWORD *exit_code)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, exit_code);

    if (This->status == SCHED_S_TASK_NOT_SCHEDULED)
    {
        *exit_code = 0;
        return SCHED_S_TASK_HAS_NOT_RUN;
    }

    *exit_code = This->exit_code;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetComment(ITask *iface, LPCWSTR comment)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(comment));

    if (!comment || !comment[0])
        comment = NULL;

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr == S_OK)
    {
        hr = IRegistrationInfo_put_Description(info, (BSTR)comment);
        IRegistrationInfo_Release(info);
        This->is_dirty = TRUE;
    }
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetComment(ITask *iface, LPWSTR *comment)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;
    BSTR description;
    DWORD len;

    TRACE("(%p, %p)\n", iface, comment);

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr != S_OK) return hr;

    hr = IRegistrationInfo_get_Description(info, &description);
    if (hr == S_OK)
    {
        len = description ? lstrlenW(description) + 1 : 1;
        *comment = CoTaskMemAlloc(len * sizeof(WCHAR));
        if (*comment)
        {
            if (!description)
                *comment[0] = 0;
            else
                lstrcpyW(*comment, description);
            hr = S_OK;
        }
        else
            hr =  E_OUTOFMEMORY;

        SysFreeString(description);
    }

    IRegistrationInfo_Release(info);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetCreator(ITask *iface, LPCWSTR creator)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(creator));

    if (!creator || !creator[0])
        creator = NULL;

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr == S_OK)
    {
        hr = IRegistrationInfo_put_Author(info, (BSTR)creator);
        IRegistrationInfo_Release(info);
        This->is_dirty = TRUE;
    }
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetCreator(ITask *iface, LPWSTR *creator)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;
    BSTR author;
    DWORD len;

    TRACE("(%p, %p)\n", iface, creator);

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr != S_OK) return hr;

    hr = IRegistrationInfo_get_Author(info, &author);
    if (hr == S_OK)
    {
        len = author ? lstrlenW(author) + 1 : 1;
        *creator = CoTaskMemAlloc(len * sizeof(WCHAR));
        if (*creator)
        {
            if (!author)
                *creator[0] = 0;
            else
                lstrcpyW(*creator, author);
            hr = S_OK;
        }
        else
            hr =  E_OUTOFMEMORY;

        SysFreeString(author);
    }

    IRegistrationInfo_Release(info);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetWorkItemData(ITask *iface, WORD count, BYTE data[])
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %u, %p)\n", iface, count, data);

    if (count)
    {
        if (!data) return E_INVALIDARG;

        free(This->data);
        This->data = malloc(count);
        if (!This->data) return E_OUTOFMEMORY;
        memcpy(This->data, data, count);
        This->data_count = count;
    }
    else
    {
        if (data) return E_INVALIDARG;

        free(This->data);
        This->data = NULL;
        This->data_count = 0;
    }

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetWorkItemData(ITask *iface, WORD *count, BYTE **data)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p, %p)\n", iface, count, data);

    if (!This->data)
    {
        *count = 0;
        *data = NULL;
    }
    else
    {
        *data = CoTaskMemAlloc(This->data_count);
        if (!*data) return E_OUTOFMEMORY;
        memcpy(*data, This->data, This->data_count);
        *count = This->data_count;
    }

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetErrorRetryCount(
        ITask* iface,
        WORD wRetryCount)
{
    FIXME("(%p, %d): stub\n", iface, wRetryCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetErrorRetryCount(ITask *iface, WORD *count)
{
    TRACE("(%p, %p)\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetErrorRetryInterval(
        ITask* iface,
        WORD wRetryInterval)
{
    FIXME("(%p, %d): stub\n", iface, wRetryInterval);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetErrorRetryInterval(ITask *iface, WORD *interval)
{
    TRACE("(%p, %p)\n", iface, interval);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetFlags(ITask *iface, DWORD flags)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, 0x%08lx)\n", iface, flags);
    This->flags &= 0xffff8000;
    This->flags |= flags & 0x7fff;
    This->is_dirty = TRUE;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetFlags(ITask *iface, DWORD *flags)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, flags);
    *flags = LOWORD(This->flags);
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetAccountInformation(
        ITask* iface,
        LPCWSTR pwszAccountName,
        LPCWSTR pwszPassword)
{
    DWORD n;
    TaskImpl *This = impl_from_ITask(iface);
    LPWSTR tmp_account_name;

    TRACE("(%p, %s, %s): partial stub\n", iface, debugstr_w(pwszAccountName),
            debugstr_w(pwszPassword));

    if (pwszPassword)
        FIXME("Partial stub ignores passwords\n");

    n = (lstrlenW(pwszAccountName) + 1);
    tmp_account_name = malloc(n * sizeof(WCHAR));
    if (!tmp_account_name)
        return E_OUTOFMEMORY;
    lstrcpyW(tmp_account_name, pwszAccountName);
    free(This->accountName);
    This->accountName = tmp_account_name;
    This->is_dirty = TRUE;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetAccountInformation(
        ITask* iface,
        LPWSTR *ppwszAccountName)
{
    DWORD n;
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p): partial stub\n", iface, ppwszAccountName);

    /* This implements the WinXP behavior when accountName has not yet
     * set.  Win2K behaves differently, returning SCHED_E_CANNOT_OPEN_TASK */
    if (!This->accountName)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    n = (lstrlenW(This->accountName) + 1);
    *ppwszAccountName = CoTaskMemAlloc(n * sizeof(WCHAR));
    if (!*ppwszAccountName)
        return E_OUTOFMEMORY;
    lstrcpyW(*ppwszAccountName, This->accountName);
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetApplicationName(ITask *iface, LPCWSTR appname)
{
    TaskImpl *This = impl_from_ITask(iface);
    DWORD len;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(appname));

    /* Empty application name */
    if (!appname || !appname[0])
        return IExecAction_put_Path(This->action, NULL);

    /* Attempt to set pwszApplicationName to a path resolved application name */
    len = SearchPathW(NULL, appname, NULL, 0, NULL, NULL);
    if (len)
    {
        LPWSTR tmp_name;

        tmp_name = malloc(len * sizeof(WCHAR));
        if (!tmp_name)
            return E_OUTOFMEMORY;
        len = SearchPathW(NULL, appname, NULL, len, tmp_name, NULL);
        if (len)
        {
            hr = IExecAction_put_Path(This->action, tmp_name);
            if (hr == S_OK) This->is_dirty = TRUE;
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

        free(tmp_name);
        return hr;
    }

    /* If unable to path resolve name, simply set to appname */
    hr = IExecAction_put_Path(This->action, (BSTR)appname);
    if (hr == S_OK) This->is_dirty = TRUE;
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetApplicationName(ITask *iface, LPWSTR *appname)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;
    BSTR path;
    DWORD len;

    TRACE("(%p, %p)\n", iface, appname);

    hr = IExecAction_get_Path(This->action, &path);
    if (hr != S_OK) return hr;

    len = path ? lstrlenW(path) + 1 : 1;
    *appname = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*appname)
    {
        if (!path)
            *appname[0] = 0;
        else
            lstrcpyW(*appname, path);
        hr = S_OK;
    }
    else
        hr =  E_OUTOFMEMORY;

    SysFreeString(path);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetParameters(ITask *iface, LPCWSTR params)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(params));

    /* Empty parameter list */
    if (!params || !params[0])
        params = NULL;

    hr = IExecAction_put_Arguments(This->action, (BSTR)params);
    if (hr == S_OK) This->is_dirty = TRUE;
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetParameters(ITask *iface, LPWSTR *params)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;
    BSTR args;
    DWORD len;

    TRACE("(%p, %p)\n", iface, params);

    hr = IExecAction_get_Arguments(This->action, &args);
    if (hr != S_OK) return hr;

    len = args ? lstrlenW(args) + 1 : 1;
    *params = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*params)
    {
        if (!args)
            *params[0] = 0;
        else
            lstrcpyW(*params, args);
        hr = S_OK;
    }
    else
        hr =  E_OUTOFMEMORY;

    SysFreeString(args);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetWorkingDirectory(ITask * iface, LPCWSTR workdir)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(workdir));

    if (!workdir || !workdir[0])
        workdir = NULL;

    hr = IExecAction_put_WorkingDirectory(This->action, (BSTR)workdir);
    if (hr == S_OK) This->is_dirty = TRUE;
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetWorkingDirectory(ITask *iface, LPWSTR *workdir)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;
    BSTR dir;
    DWORD len;

    TRACE("(%p, %p)\n", iface, workdir);

    hr = IExecAction_get_WorkingDirectory(This->action, &dir);
    if (hr != S_OK) return hr;

    len = dir ? lstrlenW(dir) + 1 : 1;
    *workdir = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*workdir)
    {
        if (!dir)
            *workdir[0] = 0;
        else
            lstrcpyW(*workdir, dir);
        hr = S_OK;
    }
    else
        hr =  E_OUTOFMEMORY;

    SysFreeString(dir);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetPriority(
        ITask* iface,
        DWORD dwPriority)
{
    FIXME("(%p, 0x%08lx): stub\n", iface, dwPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetPriority(ITask *iface, DWORD *priority)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, priority);

    *priority = This->priority;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetTaskFlags(
        ITask* iface,
        DWORD dwFlags)
{
    FIXME("(%p, 0x%08lx): stub\n", iface, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTaskFlags(ITask *iface, DWORD *flags)
{
    FIXME("(%p, %p): stub\n", iface, flags);
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetMaxRunTime(
        ITask* iface,
        DWORD dwMaxRunTime)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %ld)\n", iface, dwMaxRunTime);

    This->maxRunTime = dwMaxRunTime;
    This->is_dirty = TRUE;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetMaxRunTime(
        ITask* iface,
        DWORD *pdwMaxRunTime)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, pdwMaxRunTime);

    *pdwMaxRunTime = This->maxRunTime;
    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_QueryInterface(
        IPersistFile* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppvObject);
    return ITask_QueryInterface(&This->ITask_iface, riid, ppvObject);
}

static ULONG WINAPI MSTASK_IPersistFile_AddRef(
        IPersistFile* iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    return ITask_AddRef(&This->ITask_iface);
}

static ULONG WINAPI MSTASK_IPersistFile_Release(
        IPersistFile* iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    return ITask_Release(&This->ITask_iface);
}

static HRESULT WINAPI MSTASK_IPersistFile_GetClassID(IPersistFile *iface, CLSID *clsid)
{
    TRACE("(%p, %p)\n", iface, clsid);

    *clsid = CLSID_CTask;
    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_IsDirty(IPersistFile *iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    TRACE("(%p)\n", iface);
    return This->is_dirty ? S_OK : S_FALSE;
}

static DWORD load_unicode_strings(ITask *task, BYTE *data, DWORD limit)
{
    DWORD i, data_size = 0;
    USHORT len;

    for (i = 0; i < 5; i++)
    {
        if (limit < sizeof(USHORT))
        {
            TRACE("invalid string %lu offset\n", i);
            break;
        }

        len = *(USHORT *)data;
        data += sizeof(USHORT);
        data_size += sizeof(USHORT);
        limit -= sizeof(USHORT);
        if (limit < len * sizeof(WCHAR))
        {
            TRACE("invalid string %lu size\n", i);
            break;
        }

        TRACE("string %lu: %s\n", i, wine_dbgstr_wn((const WCHAR *)data, len));

        switch (i)
        {
        case 0:
            ITask_SetApplicationName(task, (const WCHAR *)data);
            break;
        case 1:
            ITask_SetParameters(task, (const WCHAR *)data);
            break;
        case 2:
            ITask_SetWorkingDirectory(task, (const WCHAR *)data);
            break;
        case 3:
            ITask_SetCreator(task, (const WCHAR *)data);
            break;
        case 4:
            ITask_SetComment(task, (const WCHAR *)data);
            break;
        default:
            break;
        }

        data += len * sizeof(WCHAR);
        data_size += len * sizeof(WCHAR);
    }

    return data_size;
}

static HRESULT load_job_data(TaskImpl *This, BYTE *data, DWORD size)
{
    ITask *task = &This->ITask_iface;
    HRESULT hr;
    const FIXDLEN_DATA *fixed;
    const SYSTEMTIME *st;
    DWORD unicode_strings_size, data_size, triggers_size;
    USHORT trigger_count, i;
    const USHORT *signature;
    TASK_TRIGGER *task_trigger;

    if (size < sizeof(*fixed))
    {
        TRACE("no space for FIXDLEN_DATA\n");
        return SCHED_E_INVALID_TASK;
    }

    fixed = (const FIXDLEN_DATA *)data;

    TRACE("product_version %04x\n", fixed->product_version);
    TRACE("file_version %04x\n", fixed->file_version);
    TRACE("uuid %s\n", wine_dbgstr_guid(&fixed->uuid));

    if (fixed->file_version != 0x0001)
        return SCHED_E_INVALID_TASK;

    This->uuid = fixed->uuid;

    TRACE("name_size_offset %04x\n", fixed->name_size_offset);
    TRACE("trigger_offset %04x\n", fixed->trigger_offset);
    TRACE("error_retry_count %u\n", fixed->error_retry_count);
    TRACE("error_retry_interval %u\n", fixed->error_retry_interval);
    TRACE("idle_deadline %u\n", fixed->idle_deadline);
    This->deadline_minutes = fixed->idle_deadline;
    TRACE("idle_wait %u\n", fixed->idle_wait);
    This->idle_minutes = fixed->idle_wait;
    TRACE("priority %08x\n", fixed->priority);
    This->priority = fixed->priority;
    TRACE("maximum_runtime %u\n", fixed->maximum_runtime);
    This->maxRunTime = fixed->maximum_runtime;
    TRACE("exit_code %#x\n", fixed->exit_code);
    This->exit_code = fixed->exit_code;
    TRACE("status %08lx\n", fixed->status);
    This->status = fixed->status;
    TRACE("flags %08x\n", fixed->flags);
    This->flags = fixed->flags;
    This->last_runtime = fixed->last_runtime;
    st = &fixed->last_runtime;
    TRACE("last_runtime %u/%u/%u wday %u %u:%02u:%02u.%03u\n",
            st->wDay, st->wMonth, st->wYear, st->wDayOfWeek,
            st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);

    /* Instance Count */
    if (size < sizeof(*fixed) + sizeof(USHORT))
    {
        TRACE("no space for instance count\n");
        return SCHED_E_INVALID_TASK;
    }

    This->instance_count = *(const USHORT *)(data + sizeof(*fixed));
    TRACE("instance count %u\n", This->instance_count);

    if (fixed->name_size_offset + sizeof(USHORT) < size)
        unicode_strings_size = load_unicode_strings(task, data + fixed->name_size_offset, size - fixed->name_size_offset);
    else
    {
        TRACE("invalid name_size_offset\n");
        return SCHED_E_INVALID_TASK;
    }
    TRACE("unicode strings end at %#lx\n", fixed->name_size_offset + unicode_strings_size);

    if (size < fixed->trigger_offset + sizeof(USHORT))
    {
        TRACE("no space for triggers count\n");
        return SCHED_E_INVALID_TASK;
    }
    trigger_count = *(const USHORT *)(data + fixed->trigger_offset);
    TRACE("trigger_count %u\n", trigger_count);
    triggers_size = size - fixed->trigger_offset - sizeof(USHORT);
    TRACE("triggers_size %lu\n", triggers_size);
    task_trigger = (TASK_TRIGGER *)(data + fixed->trigger_offset + sizeof(USHORT));

    data += fixed->name_size_offset + unicode_strings_size;
    size -= fixed->name_size_offset + unicode_strings_size;

    /* User Data */
    if (size < sizeof(USHORT))
    {
        TRACE("no space for user data size\n");
        return SCHED_E_INVALID_TASK;
    }

    data_size = *(const USHORT *)data;
    if (size < sizeof(USHORT) + data_size)
    {
        TRACE("no space for user data\n");
        return SCHED_E_INVALID_TASK;
    }
    TRACE("User Data size %#lx\n", data_size);
    ITask_SetWorkItemData(task, data_size, data + sizeof(USHORT));

    size -= sizeof(USHORT) + data_size;
    data += sizeof(USHORT) + data_size;

    /* Reserved Data */
    if (size < sizeof(USHORT))
    {
        TRACE("no space for reserved data size\n");
        return SCHED_E_INVALID_TASK;
    }

    data_size = *(const USHORT *)data;
    if (size < sizeof(USHORT) + data_size)
    {
        TRACE("no space for reserved data\n");
        return SCHED_E_INVALID_TASK;
    }
    TRACE("Reserved Data size %#lx\n", data_size);

    size -= sizeof(USHORT) + data_size;
    data += sizeof(USHORT) + data_size;

    /* Trigger Data */
    TRACE("trigger_offset %04x, triggers end at %04Ix\n", fixed->trigger_offset,
          fixed->trigger_offset + sizeof(USHORT) + trigger_count * sizeof(TASK_TRIGGER));

    task_trigger = (TASK_TRIGGER *)(data + sizeof(USHORT));

    if (trigger_count * sizeof(TASK_TRIGGER) > triggers_size)
    {
        TRACE("no space for triggers data\n");
        return SCHED_E_INVALID_TASK;
    }

    This->trigger_count = 0;

    for (i = 0; i < trigger_count; i++)
    {
        ITaskTrigger *trigger;
        WORD idx;

        hr = ITask_CreateTrigger(task, &idx, &trigger);
        if (hr != S_OK) return hr;

        hr = ITaskTrigger_SetTrigger(trigger, &task_trigger[i]);
        ITaskTrigger_Release(trigger);
        if (hr != S_OK)
        {
            ITask_DeleteTrigger(task, idx);
            return hr;
        }
    }

    size -= sizeof(USHORT) + trigger_count * sizeof(TASK_TRIGGER);
    data += sizeof(USHORT) + trigger_count * sizeof(TASK_TRIGGER);

    if (size < 2 * sizeof(USHORT) + 64)
    {
        TRACE("no space for signature\n");
        return S_OK; /* signature is optional */
    }

    signature = (const USHORT *)data;
    TRACE("signature version %04x, client version %04x\n", signature[0], signature[1]);

    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_Load(IPersistFile *iface, LPCOLESTR file_name, DWORD mode)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    HRESULT hr;
    HANDLE file, mapping;
    DWORD access, sharing, size, try;
    void *data;

    TRACE("(%p, %s, 0x%08lx)\n", iface, debugstr_w(file_name), mode);

    switch (mode & 0x000f)
    {
    default:
    case STGM_READ:
        access = GENERIC_READ;
        break;
    case STGM_WRITE:
    case STGM_READWRITE:
        access = GENERIC_READ | GENERIC_WRITE;
        break;
    }

    switch (mode & 0x00f0)
    {
    default:
    case STGM_SHARE_DENY_NONE:
        sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
        break;
    case STGM_SHARE_DENY_READ:
        sharing = FILE_SHARE_WRITE;
        break;
    case STGM_SHARE_DENY_WRITE:
        sharing = FILE_SHARE_READ;
        break;
    case STGM_SHARE_EXCLUSIVE:
        sharing = 0;
        break;
    }

    try = 1;
    for (;;)
    {
        file = CreateFileW(file_name, access, sharing, NULL, OPEN_EXISTING, 0, 0);
        if (file != INVALID_HANDLE_VALUE) break;

        if (GetLastError() != ERROR_SHARING_VIOLATION || try++ >= 3)
        {
            TRACE("Failed to open %s, error %lu\n", debugstr_w(file_name), GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
        Sleep(100);
    }

    size = GetFileSize(file, NULL);

    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, 0);
    if (!mapping)
    {
        TRACE("Failed to create file mapping %s, error %lu\n", debugstr_w(file_name), GetLastError());
        CloseHandle(file);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    data = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (data)
    {
        hr = load_job_data(This, data, size);
        if (hr == S_OK) This->is_dirty = FALSE;
        UnmapViewOfFile(data);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    CloseHandle(mapping);
    CloseHandle(file);

    return hr;
}

static BOOL write_signature(HANDLE hfile)
{
    struct
    {
        USHORT SignatureVersion;
        USHORT ClientVersion;
        BYTE md5[64];
    } signature;
    DWORD size;

    signature.SignatureVersion = 0x0001;
    signature.ClientVersion = 0x0001;
    memset(&signature.md5, 0, sizeof(signature.md5));

    return WriteFile(hfile, &signature, sizeof(signature), &size, NULL);
}

static BOOL write_reserved_data(HANDLE hfile)
{
    static const struct
    {
        USHORT size;
        BYTE data[8];
    } user = { 8, { 0xff,0x0f,0x1d,0,0,0,0,0 } };
    DWORD size;

    return WriteFile(hfile, &user, sizeof(user), &size, NULL);
}

static BOOL write_user_data(HANDLE hfile, BYTE *data, WORD data_size)
{
    DWORD size;

    if (!WriteFile(hfile, &data_size, sizeof(data_size), &size, NULL))
        return FALSE;

    if (!data_size) return TRUE;

    return WriteFile(hfile, data, data_size, &size, NULL);
}

static HRESULT write_triggers(TaskImpl *This, HANDLE hfile)
{
    WORD count, i, idx = 0xffff;
    DWORD size;
    HRESULT hr = S_OK;
    ITaskTrigger *trigger;

    count = This->trigger_count;

    /* Windows saves a .job with at least 1 trigger */
    if (!count)
    {
        hr = ITask_CreateTrigger(&This->ITask_iface, &idx, &trigger);
        if (hr != S_OK) return hr;
        ITaskTrigger_Release(trigger);

        count = 1;
    }

    if (WriteFile(hfile, &count, sizeof(count), &size, NULL))
    {
        for (i = 0; i < count; i++)
        {
            if (!WriteFile(hfile, &This->trigger[i], sizeof(This->trigger[0]), &size, NULL))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
        }
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    if (idx != 0xffff)
        ITask_DeleteTrigger(&This->ITask_iface, idx);

    return hr;
}

static BOOL write_unicode_string(HANDLE hfile, const WCHAR *str)
{
    USHORT count;
    DWORD size;

    count = str ? (lstrlenW(str) + 1) : 0;
    if (!WriteFile(hfile, &count, sizeof(count), &size, NULL))
        return FALSE;

    if (!str) return TRUE;

    count *= sizeof(WCHAR);
    return WriteFile(hfile, str, count, &size, NULL);
}

static HRESULT WINAPI MSTASK_IPersistFile_Save(IPersistFile *iface, LPCOLESTR task_name, BOOL remember)
{
    static WCHAR authorW[] = L"Wine";
    static WCHAR commentW[] = L"Created by Wine";
    FIXDLEN_DATA fixed;
    WORD word, user_data_size = 0;
    HANDLE hfile;
    DWORD size, ver, disposition, try;
    TaskImpl *This = impl_from_IPersistFile(iface);
    ITask *task = &This->ITask_iface;
    LPWSTR appname = NULL, params = NULL, workdir = NULL, creator = NULL, comment = NULL;
    BYTE *user_data = NULL;
    HRESULT hr;

    TRACE("(%p, %s, %d)\n", iface, debugstr_w(task_name), remember);

    disposition = task_name ? CREATE_NEW : OPEN_ALWAYS;

    if (!task_name)
    {
        task_name = This->task_name;
        remember = FALSE;
    }

    ITask_GetComment(task, &comment);
    if (!comment) comment = commentW;
    ITask_GetCreator(task, &creator);
    if (!creator) creator = authorW;
    ITask_GetApplicationName(task, &appname);
    ITask_GetParameters(task, &params);
    ITask_GetWorkingDirectory(task, &workdir);
    ITask_GetWorkItemData(task, &user_data_size, &user_data);

    ver = GetVersion();
    fixed.product_version = MAKEWORD(ver >> 8, ver);
    fixed.file_version = 0x0001;
    fixed.name_size_offset = sizeof(fixed) + sizeof(USHORT); /* FIXDLEN_DATA + Instance Count */
    fixed.trigger_offset = sizeof(fixed) + sizeof(USHORT); /* FIXDLEN_DATA + Instance Count */
    fixed.trigger_offset += sizeof(USHORT); /* Application Name */
    if (appname)
        fixed.trigger_offset += (lstrlenW(appname) + 1) * sizeof(WCHAR);
    fixed.trigger_offset += sizeof(USHORT); /* Parameters */
    if (params)
        fixed.trigger_offset += (lstrlenW(params) + 1) * sizeof(WCHAR);
    fixed.trigger_offset += sizeof(USHORT); /* Working Directory */
    if (workdir)
        fixed.trigger_offset += (lstrlenW(workdir) + 1) * sizeof(WCHAR);
    fixed.trigger_offset += sizeof(USHORT); /* Author */
    if (creator)
        fixed.trigger_offset += (lstrlenW(creator) + 1) * sizeof(WCHAR);
    fixed.trigger_offset += sizeof(USHORT); /* Comment */
    if (comment)
        fixed.trigger_offset += (lstrlenW(comment) + 1) * sizeof(WCHAR);
    fixed.trigger_offset += sizeof(USHORT) + user_data_size; /* User Data */
    fixed.trigger_offset += 10; /* Reserved Data */

    fixed.error_retry_count = 0;
    fixed.error_retry_interval = 0;
    fixed.idle_wait = This->idle_minutes;
    fixed.idle_deadline = This->deadline_minutes;
    fixed.priority = This->priority;
    fixed.maximum_runtime = This->maxRunTime;
    fixed.exit_code = This->exit_code;
    if (This->status == SCHED_S_TASK_NOT_SCHEDULED && This->trigger_count)
        This->status = SCHED_S_TASK_HAS_NOT_RUN;
    fixed.status = This->status;
    fixed.flags = This->flags;
    fixed.last_runtime = This->last_runtime;

    try = 1;
    for (;;)
    {
        hfile = CreateFileW(task_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, disposition, 0, 0);
        if (hfile != INVALID_HANDLE_VALUE) break;

        if (try++ >= 3)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto failed;
        }
        Sleep(100);
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
        fixed.uuid = This->uuid;
    else
        CoCreateGuid(&fixed.uuid);

    if (!WriteFile(hfile, &fixed, sizeof(fixed), &size, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    /* Instance Count: don't touch it in the client */
    if (SetFilePointer(hfile, sizeof(word), NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }
    /* Application Name */
    if (!write_unicode_string(hfile, appname))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }
    /* Parameters */
    if (!write_unicode_string(hfile, params))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }
    /* Working Directory */
    if (!write_unicode_string(hfile, workdir))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }
    /* Author */
    if (!write_unicode_string(hfile, creator))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }
    /* Comment */
    if (!write_unicode_string(hfile, comment))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    /* User Data */
    if (!write_user_data(hfile, user_data, user_data_size))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    /* Reserved Data */
    if (!write_reserved_data(hfile))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    /* Triggers */
    hr = write_triggers(This, hfile);
    if (hr != S_OK)
        goto failed;

    /* Signature */
    if (!write_signature(hfile))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    hr = S_OK;
    This->is_dirty = FALSE;

failed:
    CoTaskMemFree(appname);
    CoTaskMemFree(params);
    CoTaskMemFree(workdir);
    if (creator != authorW)
        CoTaskMemFree(creator);
    if (comment != commentW)
        CoTaskMemFree(comment);
    CoTaskMemFree(user_data);

    if (hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
        if (hr != S_OK)
            DeleteFileW(task_name);
        else if (remember)
        {
            free(This->task_name);
            This->task_name = wcsdup(task_name);
        }
    }
    return hr;
}

static HRESULT WINAPI MSTASK_IPersistFile_SaveCompleted(
        IPersistFile* iface,
        LPCOLESTR pszFileName)
{
    FIXME("(%p, %p): stub\n", iface, pszFileName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *file_name)
{
    TaskImpl *This = impl_from_IPersistFile(iface);

    TRACE("(%p, %p)\n", iface, file_name);

    *file_name = CoTaskMemAlloc((lstrlenW(This->task_name) + 1) * sizeof(WCHAR));
    if (!*file_name) return E_OUTOFMEMORY;

    lstrcpyW(*file_name, This->task_name);
    return S_OK;
}

static const ITaskVtbl MSTASK_ITaskVtbl =
{
    MSTASK_ITask_QueryInterface,
    MSTASK_ITask_AddRef,
    MSTASK_ITask_Release,
    MSTASK_ITask_CreateTrigger,
    MSTASK_ITask_DeleteTrigger,
    MSTASK_ITask_GetTriggerCount,
    MSTASK_ITask_GetTrigger,
    MSTASK_ITask_GetTriggerString,
    MSTASK_ITask_GetRunTimes,
    MSTASK_ITask_GetNextRunTime,
    MSTASK_ITask_SetIdleWait,
    MSTASK_ITask_GetIdleWait,
    MSTASK_ITask_Run,
    MSTASK_ITask_Terminate,
    MSTASK_ITask_EditWorkItem,
    MSTASK_ITask_GetMostRecentRunTime,
    MSTASK_ITask_GetStatus,
    MSTASK_ITask_GetExitCode,
    MSTASK_ITask_SetComment,
    MSTASK_ITask_GetComment,
    MSTASK_ITask_SetCreator,
    MSTASK_ITask_GetCreator,
    MSTASK_ITask_SetWorkItemData,
    MSTASK_ITask_GetWorkItemData,
    MSTASK_ITask_SetErrorRetryCount,
    MSTASK_ITask_GetErrorRetryCount,
    MSTASK_ITask_SetErrorRetryInterval,
    MSTASK_ITask_GetErrorRetryInterval,
    MSTASK_ITask_SetFlags,
    MSTASK_ITask_GetFlags,
    MSTASK_ITask_SetAccountInformation,
    MSTASK_ITask_GetAccountInformation,
    MSTASK_ITask_SetApplicationName,
    MSTASK_ITask_GetApplicationName,
    MSTASK_ITask_SetParameters,
    MSTASK_ITask_GetParameters,
    MSTASK_ITask_SetWorkingDirectory,
    MSTASK_ITask_GetWorkingDirectory,
    MSTASK_ITask_SetPriority,
    MSTASK_ITask_GetPriority,
    MSTASK_ITask_SetTaskFlags,
    MSTASK_ITask_GetTaskFlags,
    MSTASK_ITask_SetMaxRunTime,
    MSTASK_ITask_GetMaxRunTime
};

static const IPersistFileVtbl MSTASK_IPersistFileVtbl =
{
    MSTASK_IPersistFile_QueryInterface,
    MSTASK_IPersistFile_AddRef,
    MSTASK_IPersistFile_Release,
    MSTASK_IPersistFile_GetClassID,
    MSTASK_IPersistFile_IsDirty,
    MSTASK_IPersistFile_Load,
    MSTASK_IPersistFile_Save,
    MSTASK_IPersistFile_SaveCompleted,
    MSTASK_IPersistFile_GetCurFile
};

HRESULT TaskConstructor(ITaskService *service, const WCHAR *name, ITask **task)
{
    TaskImpl *This;
    WCHAR task_name[MAX_PATH];
    ITaskDefinition *taskdef;
    IActionCollection *actions;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_w(name), task);

    if (wcschr(name, '.')) return E_INVALIDARG;

    GetWindowsDirectoryW(task_name, MAX_PATH);
    lstrcatW(task_name, L"\\Tasks\\");
    lstrcatW(task_name, name);
    lstrcatW(task_name, L".job");

    hr = ITaskService_NewTask(service, 0, &taskdef);
    if (hr != S_OK) return hr;

    This = malloc(sizeof(*This));
    if (!This)
    {
        ITaskDefinition_Release(taskdef);
        return E_OUTOFMEMORY;
    }

    This->ITask_iface.lpVtbl = &MSTASK_ITaskVtbl;
    This->IPersistFile_iface.lpVtbl = &MSTASK_IPersistFileVtbl;
    This->ref = 1;
    This->task = taskdef;
    This->data = NULL;
    This->data_count = 0;
    This->task_name = wcsdup(task_name);
    This->flags = 0;
    This->status = SCHED_S_TASK_NOT_SCHEDULED;
    This->exit_code = 0;
    This->idle_minutes = 10;
    This->deadline_minutes = 60;
    This->priority = NORMAL_PRIORITY_CLASS;
    This->accountName = NULL;
    This->trigger_count = 0;
    This->trigger = NULL;
    This->is_dirty = FALSE;
    This->instance_count = 0;

    memset(&This->last_runtime, 0, sizeof(This->last_runtime));
    CoCreateGuid(&This->uuid);

    /* Default time is 3 days = 259200000 ms */
    This->maxRunTime = 259200000;

    hr = ITaskDefinition_get_Actions(This->task, &actions);
    if (hr == S_OK)
    {
        hr = IActionCollection_Create(actions, TASK_ACTION_EXEC, (IAction **)&This->action);
        IActionCollection_Release(actions);
        if (hr == S_OK)
        {
            *task = &This->ITask_iface;
            InterlockedIncrement(&dll_ref);
            return S_OK;
        }
    }

    ITaskDefinition_Release(This->task);
    ITask_Release(&This->ITask_iface);
    return hr;
}
