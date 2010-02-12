/*
 * Copyright (C) 2008 Google (Roy Shea)
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
#include "winternl.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

static HRESULT WINAPI MSTASK_ITaskTrigger_QueryInterface(
        ITaskTrigger* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskTriggerImpl *This = (TaskTriggerImpl *)iface;

    TRACE("IID: %s\n", debugstr_guid(riid));
    if (ppvObject == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITaskTrigger))
    {
        *ppvObject = &This->lpVtbl;
        ITaskTrigger_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITaskTrigger_AddRef(
        ITaskTrigger* iface)
{
    TaskTriggerImpl *This = (TaskTriggerImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_ITaskTrigger_Release(
        ITaskTrigger* iface)
{
    TaskTriggerImpl *This = (TaskTriggerImpl *)iface;
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
        InterlockedDecrement(&dll_ref);
    }
    return ref;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_SetTrigger(
        ITaskTrigger* iface,
        const PTASK_TRIGGER pTrigger)
{
    TaskTriggerImpl * This = (TaskTriggerImpl *)iface;
    TIME_FIELDS field_time;
    LARGE_INTEGER sys_time;
    TASK_TRIGGER tmp_trigger_cond;

    TRACE("(%p, %p)\n", iface, pTrigger);

    /* Verify valid structure size */
    if (pTrigger->cbTriggerSize != sizeof(*pTrigger))
        return E_INVALIDARG;
    tmp_trigger_cond.cbTriggerSize = pTrigger->cbTriggerSize;

    /* Reserved field must be zero */
    tmp_trigger_cond.Reserved1 = 0;

    /* Verify and set valid start date and time */
    memset(&field_time, 0, sizeof(field_time));
    field_time.Year = pTrigger->wBeginYear;
    field_time.Month = pTrigger->wBeginMonth;
    field_time.Day = pTrigger->wBeginDay;
    field_time.Hour = pTrigger->wStartHour;
    field_time.Minute = pTrigger->wStartMinute;
    if (!RtlTimeFieldsToTime(&field_time, &sys_time))
        return E_INVALIDARG;
    tmp_trigger_cond.wBeginYear = pTrigger->wBeginYear;
    tmp_trigger_cond.wBeginMonth = pTrigger->wBeginMonth;
    tmp_trigger_cond.wBeginDay = pTrigger->wBeginDay;
    tmp_trigger_cond.wStartHour = pTrigger->wStartHour;
    tmp_trigger_cond.wStartMinute = pTrigger->wStartMinute;

    /* Verify valid end date if TASK_TRIGGER_FLAG_HAS_END_DATE flag is set */
    if (pTrigger->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE)
    {
        memset(&field_time, 0, sizeof(field_time));
        field_time.Year = pTrigger->wEndYear;
        field_time.Month = pTrigger->wEndMonth;
        field_time.Day = pTrigger->wEndDay;
        if (!RtlTimeFieldsToTime(&field_time, &sys_time))
            return E_INVALIDARG;
    }

    /* Set valid end date independent of TASK_TRIGGER_FLAG_HAS_END_DATE flag */
    tmp_trigger_cond.wEndYear = pTrigger->wEndYear;
    tmp_trigger_cond.wEndMonth = pTrigger->wEndMonth;
    tmp_trigger_cond.wEndDay = pTrigger->wEndDay;

    /* Verify duration and interval pair */
    if (pTrigger->MinutesDuration <= pTrigger->MinutesInterval &&
            pTrigger->MinutesInterval > 0)
        return E_INVALIDARG;
    if (pTrigger->MinutesDuration > 0 && pTrigger->MinutesInterval == 0)
        return E_INVALIDARG;
    tmp_trigger_cond.MinutesDuration = pTrigger->MinutesDuration;
    tmp_trigger_cond.MinutesInterval = pTrigger->MinutesInterval;

    /* Copy over flags */
    tmp_trigger_cond.rgFlags = pTrigger->rgFlags;

    /* Set TriggerType dependent fields of Type union */
    tmp_trigger_cond.TriggerType = pTrigger->TriggerType;
    switch (pTrigger->TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
            tmp_trigger_cond.Type.Daily.DaysInterval =
                    pTrigger->Type.Daily.DaysInterval;
            break;
        case TASK_TIME_TRIGGER_WEEKLY:
            tmp_trigger_cond.Type.Weekly.WeeksInterval =
                    pTrigger->Type.Weekly.WeeksInterval;
            tmp_trigger_cond.Type.Weekly.rgfDaysOfTheWeek =
                    pTrigger->Type.Weekly.rgfDaysOfTheWeek;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDATE:
            tmp_trigger_cond.Type.MonthlyDate.rgfDays =
                    pTrigger->Type.MonthlyDate.rgfDays;
            tmp_trigger_cond.Type.MonthlyDate.rgfMonths =
                    pTrigger->Type.MonthlyDate.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDOW:
            tmp_trigger_cond.Type.MonthlyDOW.wWhichWeek =
                    pTrigger->Type.MonthlyDOW.wWhichWeek;
            tmp_trigger_cond.Type.MonthlyDOW.rgfDaysOfTheWeek =
                    pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek;
            tmp_trigger_cond.Type.MonthlyDOW.rgfMonths =
                    pTrigger->Type.MonthlyDOW.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_ONCE:
        case TASK_EVENT_TRIGGER_ON_IDLE:
        case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        case TASK_EVENT_TRIGGER_AT_LOGON:
        default:
            tmp_trigger_cond.Type = This->triggerCond.Type;
            break;
    }

    /* Reserved field must be zero */
    tmp_trigger_cond.Reserved2 = 0;

    /* wRandomMinutesInterval not currently used and is initialized to zero */
    tmp_trigger_cond.wRandomMinutesInterval = 0;

    /* Update object copy of triggerCond */
    This->triggerCond = tmp_trigger_cond;

    return S_OK;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_GetTrigger(
        ITaskTrigger* iface,
        PTASK_TRIGGER pTrigger)
{
    TaskTriggerImpl * This = (TaskTriggerImpl *)iface;

    TRACE("(%p, %p)\n", iface, pTrigger);

    /* Native implementation doesn't verify equivalent cbTriggerSize fields */

    /* Copy relevant fields of the structure */
    pTrigger->cbTriggerSize = This->triggerCond.cbTriggerSize;
    pTrigger->Reserved1 = 0;
    pTrigger->wBeginYear = This->triggerCond.wBeginYear;
    pTrigger->wBeginMonth = This->triggerCond.wBeginMonth;
    pTrigger->wBeginDay = This->triggerCond.wBeginDay;
    pTrigger->wEndYear = This->triggerCond.wEndYear;
    pTrigger->wEndMonth = This->triggerCond.wEndMonth;
    pTrigger->wEndDay = This->triggerCond.wEndDay;
    pTrigger->wStartHour = This->triggerCond.wStartHour;
    pTrigger->wStartMinute = This->triggerCond.wStartMinute;
    pTrigger->MinutesDuration = This->triggerCond.MinutesDuration;
    pTrigger->MinutesInterval = This->triggerCond.MinutesInterval;
    pTrigger->rgFlags = This->triggerCond.rgFlags;
    pTrigger->TriggerType = This->triggerCond.TriggerType;
    switch (This->triggerCond.TriggerType)
    {
        case TASK_TIME_TRIGGER_DAILY:
            pTrigger->Type.Daily.DaysInterval =
                    This->triggerCond.Type.Daily.DaysInterval;
            break;
        case TASK_TIME_TRIGGER_WEEKLY:
            pTrigger->Type.Weekly.WeeksInterval =
                    This->triggerCond.Type.Weekly.WeeksInterval;
            pTrigger->Type.Weekly.rgfDaysOfTheWeek =
                    This->triggerCond.Type.Weekly.rgfDaysOfTheWeek;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDATE:
            pTrigger->Type.MonthlyDate.rgfDays =
                    This->triggerCond.Type.MonthlyDate.rgfDays;
            pTrigger->Type.MonthlyDate.rgfMonths =
                    This->triggerCond.Type.MonthlyDate.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_MONTHLYDOW:
            pTrigger->Type.MonthlyDOW.wWhichWeek =
                    This->triggerCond.Type.MonthlyDOW.wWhichWeek;
            pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek =
                    This->triggerCond.Type.MonthlyDOW.rgfDaysOfTheWeek;
            pTrigger->Type.MonthlyDOW.rgfMonths =
                    This->triggerCond.Type.MonthlyDOW.rgfMonths;
            break;
        case TASK_TIME_TRIGGER_ONCE:
        case TASK_EVENT_TRIGGER_ON_IDLE:
        case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
        case TASK_EVENT_TRIGGER_AT_LOGON:
        default:
            break;
    }
    pTrigger->Reserved2 = 0;
    pTrigger->wRandomMinutesInterval = 0;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITaskTrigger_GetTriggerString(
        ITaskTrigger* iface,
        LPWSTR *ppwszTrigger)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static const ITaskTriggerVtbl MSTASK_ITaskTriggerVtbl =
{
    MSTASK_ITaskTrigger_QueryInterface,
    MSTASK_ITaskTrigger_AddRef,
    MSTASK_ITaskTrigger_Release,
    MSTASK_ITaskTrigger_SetTrigger,
    MSTASK_ITaskTrigger_GetTrigger,
    MSTASK_ITaskTrigger_GetTriggerString
};

HRESULT TaskTriggerConstructor(LPVOID *ppObj)
{
    TaskTriggerImpl *This;
    SYSTEMTIME time;
    TRACE("(%p)\n", ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &MSTASK_ITaskTriggerVtbl;
    This->ref = 1;

    /* Most fields of triggerCond default to zero.  Initialize other
     * fields to default values. */
    memset(&This->triggerCond, 0, sizeof(TASK_TRIGGER));
    GetLocalTime(&time);
    This->triggerCond.cbTriggerSize = sizeof(This->triggerCond);
    This->triggerCond.wBeginYear = time.wYear;
    This->triggerCond.wBeginMonth = time.wMonth;
    This->triggerCond.wBeginDay = time.wDay;
    This->triggerCond.wStartHour = time.wHour;
    This->triggerCond.wStartMinute = time.wMinute;
    This->triggerCond.rgFlags = TASK_TRIGGER_FLAG_DISABLED;
    This->triggerCond.TriggerType = TASK_TIME_TRIGGER_DAILY,
    This->triggerCond.Type.Daily.DaysInterval = 1;

    *ppObj = &This->lpVtbl;
    InterlockedIncrement(&dll_ref);
    return S_OK;
}
