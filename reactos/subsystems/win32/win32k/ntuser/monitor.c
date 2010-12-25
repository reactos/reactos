/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Monitor support
 *  FILE:             subsys/win32k/ntuser/monitor.c
 *  PROGRAMER:        Anich Gregor (blight@blight.eu.org)
 *  REVISION HISTORY:
 *       26-02-2004  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

/* FIXME: find include file for these */
#define MONITORINFOF_PRIMARY      1
#define MONITOR_DEFAULTTONULL     0
#define MONITOR_DEFAULTTOPRIMARY  1
#define MONITOR_DEFAULTTONEAREST  2

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* list of monitors */
static PMONITOR gMonitorList = NULL;

/* INITALIZATION FUNCTIONS ****************************************************/

INIT_FUNCTION
NTSTATUS
NTAPI
InitMonitorImpl()
{
    DPRINT("Initializing monitor implementation...\n");

    return STATUS_SUCCESS;
}

NTSTATUS
CleanupMonitorImpl()
{
    DPRINT("Cleaning up monitor implementation...\n");
    /* FIXME: Destroy monitor objects? */

    return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS **********************************************************/

/* IntCreateMonitorObject
 *
 * Creates a MONITOR
 *
 * Return value
 *   If the function succeeds a pointer to a MONITOR is returned. On failure
 *   NULL is returned.
 */
static
PMONITOR
IntCreateMonitorObject()
{
    HANDLE Handle;
    PMONITOR Monitor;

    Monitor = UserCreateObject(gHandleTable, NULL, &Handle, otMonitor, sizeof (MONITOR));
    if (Monitor == NULL)
    {
        return NULL;
    }

    ExInitializeFastMutex(&Monitor->Lock);

    return Monitor;
}

/* IntDestroyMonitorObject
 *
 * Destroys a MONITOR
 * You have to be the owner of the monitors lock to safely destroy it.
 *
 * Arguments
 *
 *   pMonitor
 *      Pointer to the MONITOR which shall be deleted
 */
static
void
IntDestroyMonitorObject(IN PMONITOR pMonitor)
{
    RtlFreeUnicodeString(&pMonitor->DeviceName);
    UserDereferenceObject(pMonitor);
}


PMONITOR FASTCALL
UserGetMonitorObject(IN HMONITOR hMonitor)
{
    PMONITOR Monitor;

    if (!hMonitor)
    {
        EngSetLastError(ERROR_INVALID_MONITOR_HANDLE);
        return NULL;
    }

    Monitor = (PMONITOR)UserGetObject(gHandleTable, hMonitor, otMonitor);
    if (!Monitor)
    {
        EngSetLastError(ERROR_INVALID_MONITOR_HANDLE);
        return NULL;
    }

    ASSERT(Monitor->head.cLockObj >= 0);

    return Monitor;
}


/* IntAttachMonitor
 *
 * Creates a new MONITOR and appends it to the list of monitors.
 *
 * Arguments
 *
 *   pGdiDevice     Pointer to the PDEVOBJ onto which the monitor was attached
 *   DisplayNumber  Display Number (starting with 0)
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS
IntAttachMonitor(IN PDEVOBJ *pGdiDevice,
                 IN ULONG DisplayNumber)
{
    PMONITOR Monitor;
    WCHAR Buffer[CCHDEVICENAME];

    DPRINT("Attaching monitor...\n");

    /* create new monitor object */
    Monitor = IntCreateMonitorObject();
    if (Monitor == NULL)
    {
        DPRINT("Couldnt create monitor object\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _snwprintf(Buffer, CCHDEVICENAME, L"\\\\.\\DISPLAY%d", DisplayNumber + 1);
    if (!RtlCreateUnicodeString(&Monitor->DeviceName, Buffer))
    {
        DPRINT("Couldn't duplicate monitor name!\n");
        UserDereferenceObject(Monitor);
        UserDeleteObject(UserHMGetHandle(Monitor), otMonitor);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Monitor->GdiDevice = pGdiDevice;
    Monitor->rcMonitor.left  = 0;
    Monitor->rcMonitor.top   = 0;
    Monitor->rcMonitor.right  = Monitor->rcMonitor.left + pGdiDevice->gdiinfo.ulHorzRes;
    Monitor->rcMonitor.bottom = Monitor->rcMonitor.top + pGdiDevice->gdiinfo.ulVertRes;
    Monitor->rcWork = Monitor->rcMonitor;
    Monitor->cWndStack = 0;

    Monitor->hrgnMonitor = IntSysCreateRectRgnIndirect( &Monitor->rcMonitor );

    IntGdiSetRegionOwner(Monitor->hrgnMonitor, GDI_OBJ_HMGR_PUBLIC);

    if (gMonitorList == NULL)
    {
        DPRINT("Primary monitor is beeing attached\n");
        Monitor->IsPrimary = TRUE;
        gMonitorList = Monitor;
    }
    else
    {
        PMONITOR p;
        DPRINT("Additional monitor is beeing attached\n");
        for (p = gMonitorList; p->Next != NULL; p = p->Next)
        {
            p->Next = Monitor;
        }
        Monitor->Prev = p;
    }

    return STATUS_SUCCESS;
}

/* IntDetachMonitor
 *
 * Deletes a MONITOR and removes it from the list of monitors.
 *
 * Arguments
 *
 *   pGdiDevice  Pointer to the PDEVOBJ from which the monitor was detached
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS
IntDetachMonitor(IN PDEVOBJ *pGdiDevice)
{
    PMONITOR Monitor;

    for (Monitor = gMonitorList; Monitor != NULL; Monitor = Monitor->Next)
    {
        if (Monitor->GdiDevice == pGdiDevice)
            break;
    }

    if (Monitor == NULL)
    {
        /* no monitor for given device found */
        return STATUS_INVALID_PARAMETER;
    }

    if (Monitor->IsPrimary && (Monitor->Next != NULL || Monitor->Prev != NULL))
    {
        PMONITOR NewPrimaryMonitor = (Monitor->Prev != NULL) ? (Monitor->Prev) : (Monitor->Next);

        ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&NewPrimaryMonitor->Lock);
        NewPrimaryMonitor->IsPrimary = TRUE;
        ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&NewPrimaryMonitor->Lock);
    }

    if (gMonitorList == Monitor)
    {
        gMonitorList = Monitor->Next;
        if (Monitor->Next != NULL)
            Monitor->Next->Prev = NULL;
    }
    else
    {
        Monitor->Prev->Next = Monitor->Next;
        if (Monitor->Next != NULL)
            Monitor->Next->Prev = Monitor->Prev;
    }

    if (Monitor->hrgnMonitor)
        REGION_FreeRgnByHandle(Monitor->hrgnMonitor);

    IntDestroyMonitorObject(Monitor);

    return STATUS_SUCCESS;
}

/* IntGetPrimaryMonitor
 *
 * Returns a PMONITOR for the primary monitor
 *
 * Return value
 *   PMONITOR
 */
PMONITOR
FASTCALL
IntGetPrimaryMonitor()
{
    PMONITOR Monitor;

    for (Monitor = gMonitorList; Monitor != NULL; Monitor = Monitor->Next)
    {
        /* FIXME: I guess locking the monitor is not neccessary to read 1 int */
        if (Monitor->IsPrimary)
            break;
    }

    return Monitor;
}

/* IntGetMonitorsFromRect
 *
 * Returns a list of monitor handles/rectangles. The rectangles in the list are
 * the areas of intersection with the monitors.
 *
 * Arguments
 *
 *   pRect
 *      Rectangle in desktop coordinates. If this is NULL all monitors are
 *      returned and the rect list is filled with the sizes of the monitors.
 *
 *   hMonitorList
 *      Pointer to an array of HMONITOR which is filled with monitor handles.
 *      Can be NULL
 *
 *   monitorRectList
 *      Pointer to an array of RECT which is filled with intersection rects in
 *      desktop coordinates.
 *      Can be NULL, will be ignored if no intersecting monitor is found and
 *      flags is MONITOR_DEFAULTTONEAREST
 *
 *   listSize
 *      Size of the hMonitorList and monitorRectList arguments. If this is zero
 *      hMonitorList and monitorRectList are ignored.
 *
 *   flags
 *      Either 0 or MONITOR_DEFAULTTONEAREST (ignored if rect is NULL)
 *
 * Returns
 *   The number of monitors which intersect the specified region.
 */
static
UINT
IntGetMonitorsFromRect(OPTIONAL IN LPCRECTL pRect,
                       OPTIONAL OUT HMONITOR *hMonitorList,
                       OPTIONAL OUT PRECTL monitorRectList,
                       OPTIONAL IN DWORD listSize,
                       OPTIONAL IN DWORD flags)
{
    PMONITOR Monitor, NearestMonitor = NULL, PrimaryMonitor = NULL;
    UINT iCount = 0;
    ULONG iNearestDistance = 0xffffffff;

    /* Find monitors which intersect the rectangle */
    for (Monitor = gMonitorList; Monitor != NULL; Monitor = Monitor->Next)
    {
        RECTL MonitorRect, IntersectionRect;

        ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&Monitor->Lock);
        MonitorRect = Monitor->rcMonitor;
        ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&Monitor->Lock);

        DPRINT("MonitorRect: left = %d, top = %d, right = %d, bottom = %d\n",
               MonitorRect.left, MonitorRect.top, MonitorRect.right, MonitorRect.bottom);

        if (flags == MONITOR_DEFAULTTOPRIMARY && Monitor->IsPrimary)
        {
            PrimaryMonitor = Monitor;
        }

        /* Check if a rect is given */
        if (pRect == NULL)
        {
            /* No rect given, so use the full monitor rect */
            IntersectionRect = MonitorRect;
        }

        /* We have a rect, calculate intersection */
        else if (!RECTL_bIntersectRect(&IntersectionRect, &MonitorRect, pRect))
        {
            /* Rects did not intersect */
            if (flags == MONITOR_DEFAULTTONEAREST)
            {
                ULONG cx, cy, iDistance;

                /* Get x and y distance */
                cx = min(abs(MonitorRect.left - pRect->right),
                         abs(pRect->left - MonitorRect.right));
                cy = min(abs(MonitorRect.top - pRect->bottom),
                         abs(pRect->top - MonitorRect.bottom));

                /* Calculate distance square */
                iDistance = cx * cx + cy * cy;

                /* Check if this is the new nearest monitor */
                if (iDistance < iNearestDistance)
                {
                    iNearestDistance = iDistance;
                    NearestMonitor = Monitor;
                }
            }

            continue;
        }

        /* Check if there's space in the buffer */
        if (iCount < listSize)
        {
            if (hMonitorList != NULL)
                hMonitorList[iCount] = UserHMGetHandle(Monitor);
            if (monitorRectList != NULL)
                monitorRectList[iCount] = IntersectionRect;
        }
        
        /* Increase count of found monitors */
        iCount++;
    }

    /* Found nothing intersecting? */
    if (iCount == 0)
    {
        /* Check if we shall default to the nearest monitor */
        if (flags == MONITOR_DEFAULTTONEAREST && NearestMonitor)
        {
            if (hMonitorList && listSize > 0)
                hMonitorList[iCount] = UserHMGetHandle(NearestMonitor);
            iCount++;
        }
        /* Check if we shall default to the primary monitor */
        else if (flags == MONITOR_DEFAULTTOPRIMARY && PrimaryMonitor)
        {
            if (hMonitorList != NULL && listSize > 0)
                hMonitorList[iCount] = UserHMGetHandle(PrimaryMonitor);
            iCount++;
        }
    }

    return iCount;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/* NtUserEnumDisplayMonitors
 *
 * Enumerates display monitors which intersect the given HDC/cliprect
 *
 * Arguments
 *
 *   hDC
 *      Handle to a DC for which to enum intersecting monitors. If this is NULL
 *      it returns all monitors which are part of the current virtual screen.
 *
 *   pRect
 *      Clipping rectangle with coordinate system origin at the DCs origin if the
 *      given HDC is not NULL or in virtual screen coordinated if it is NULL.
 *      Can be NULL
 *
 *   hMonitorList
 *      Pointer to an array of HMONITOR which is filled with monitor handles.
 *      Can be NULL
 *
 *   monitorRectList
 *      Pointer to an array of RECT which is filled with intersection rectangles.
 *      Can be NULL
 *
 *   listSize
 *      Size of the hMonitorList and monitorRectList arguments. If this is zero
 *      hMonitorList and monitorRectList are ignored.
 *
 * Returns
 *   The number of monitors which intersect the specified region or -1 on failure.
 */
INT
APIENTRY
NtUserEnumDisplayMonitors(
    OPTIONAL IN HDC hDC,
    OPTIONAL IN LPCRECTL pRect,
    OPTIONAL OUT HMONITOR *hMonitorList,
    OPTIONAL OUT PRECTL monitorRectList,
    OPTIONAL IN DWORD listSize)
{
    INT numMonitors, i;
    HMONITOR *safeHMonitorList = NULL;
    PRECTL safeRectList = NULL;
    RECTL rect, *myRect;
    RECTL dcRect;
    NTSTATUS status;

    /* get rect */
    if (pRect != NULL)
    {
        status = MmCopyFromCaller(&rect, pRect, sizeof (RECT));
        if (!NT_SUCCESS(status))
        {
            DPRINT("MmCopyFromCaller() failed!\n");
            SetLastNtError(status);
            return -1;
        }
    }

    if (hDC != NULL)
    {
        PDC dc;
        INT regionType;

        /* get visible region bounding rect */
        dc = DC_LockDc(hDC);
        if (dc == NULL)
        {
            DPRINT("DC_LockDc() failed!\n");
            /* FIXME: setlasterror? */
            return -1;
        }
        regionType = REGION_GetRgnBox(dc->prgnVis, &dcRect);
        DC_UnlockDc(dc);

        if (regionType == 0)
        {
            DPRINT("NtGdiGetRgnBox() failed!\n");
            return -1;
        }
        if (regionType == NULLREGION)
            return 0;
        if (regionType == COMPLEXREGION)
        {
            /* TODO: warning */
        }

        /* if hDC and pRect are given the area of interest is pRect with
           coordinate origin at the DC position */
        if (pRect != NULL)
        {
            rect.left += dcRect.left;
            rect.right += dcRect.left;
            rect.top += dcRect.top;
            rect.bottom += dcRect.top;
        }
        /* if hDC is given and pRect is not the area of interest is the
           bounding rect of hDC */
        else
        {
            rect = dcRect;
        }
    }

    if (hDC == NULL && pRect == NULL)
        myRect = NULL;
    else
        myRect = &rect;

    /* find intersecting monitors */
    numMonitors = IntGetMonitorsFromRect(myRect, NULL, NULL, 0, 0);
    if (numMonitors == 0 || listSize == 0 ||
            (hMonitorList == NULL && monitorRectList == NULL))
    {
        DPRINT("numMonitors = %d\n", numMonitors);
        return numMonitors;
    }

    if (hMonitorList != NULL && listSize != 0)
    {
        safeHMonitorList = ExAllocatePoolWithTag(PagedPool, sizeof (HMONITOR) * listSize, USERTAG_MONITORRECTS);
        if (safeHMonitorList == NULL)
        {
            /* FIXME: EngSetLastError? */
            return -1;
        }
    }
    if (monitorRectList != NULL && listSize != 0)
    {
        safeRectList = ExAllocatePoolWithTag(PagedPool, sizeof (RECT) * listSize, USERTAG_MONITORRECTS);
        if (safeRectList == NULL)
        {
            ExFreePoolWithTag(safeHMonitorList, USERTAG_MONITORRECTS);
            /* FIXME: EngSetLastError? */
            return -1;
        }
    }

    /* get intersecting monitors */
    numMonitors = IntGetMonitorsFromRect(myRect, safeHMonitorList, safeRectList,
                                         listSize, 0 );

    if (hDC != NULL && pRect != NULL && safeRectList != NULL)
        for (i = 0; i < numMonitors; i++)
        {
            safeRectList[i].left -= dcRect.left;
            safeRectList[i].right -= dcRect.left;
            safeRectList[i].top -= dcRect.top;
            safeRectList[i].bottom -= dcRect.top;
        }

    /* output result */
    if (hMonitorList != NULL && listSize != 0)
    {
        status = MmCopyToCaller(hMonitorList, safeHMonitorList, sizeof (HMONITOR) * listSize);
        ExFreePool(safeHMonitorList);
        if (!NT_SUCCESS(status))
        {
            ExFreePoolWithTag(safeRectList, USERTAG_MONITORRECTS);
            SetLastNtError(status);
            return -1;
        }
    }
    if (monitorRectList != NULL && listSize != 0)
    {
        status = MmCopyToCaller(monitorRectList, safeRectList, sizeof (RECT) * listSize);
        ExFreePoolWithTag(safeRectList, USERTAG_MONITORRECTS);
        if (!NT_SUCCESS(status))
        {
            SetLastNtError(status);
            return -1;
        }
    }

    return numMonitors;
}

/* NtUserGetMonitorInfo
 *
 * Retrieves information about a given monitor
 *
 * Arguments
 *
 *   hMonitor
 *      Handle to a monitor for which to get information
 *
 *   pMonitorInfo
 *      Pointer to a MONITORINFO struct which is filled with the information.
 *      The cbSize member must be set to sizeof(MONITORINFO) or
 *      sizeof(MONITORINFOEX). Even if set to sizeof(MONITORINFOEX) only parts
 *      from MONITORINFO will be filled.
 *
 *   pDevice
 *      Pointer to a UNICODE_STRING which will recieve the device's name. The
 *      length should be CCHDEVICENAME
 *      Can be NULL
 *
 * Return value
 *   TRUE on success; FALSE on failure (calls SetLastNtError())
 *
 */
BOOL
APIENTRY
NtUserGetMonitorInfo(
    IN HMONITOR hMonitor,
    OUT LPMONITORINFO pMonitorInfo)
{
    PMONITOR Monitor;
    MONITORINFOEXW MonitorInfo;
    NTSTATUS Status;
    DECLARE_RETURN(BOOL);

    DPRINT("Enter NtUserGetMonitorInfo\n");
    UserEnterShared();

    /* get monitor object */
    if (!(Monitor = UserGetMonitorObject(hMonitor)))
    {
        DPRINT("Couldnt find monitor 0x%lx\n", hMonitor);
        RETURN(FALSE);
    }

    if(pMonitorInfo == NULL)
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        RETURN(FALSE);
    }

    /* get size of pMonitorInfo */
    Status = MmCopyFromCaller(&MonitorInfo.cbSize, &pMonitorInfo->cbSize, sizeof (MonitorInfo.cbSize));
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        RETURN(FALSE);
    }
    if ((MonitorInfo.cbSize != sizeof (MONITORINFO)) &&
            (MonitorInfo.cbSize != sizeof (MONITORINFOEXW)))
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        RETURN(FALSE);
    }

    /* fill monitor info */
    MonitorInfo.rcMonitor = Monitor->rcMonitor;
    MonitorInfo.rcWork = Monitor->rcWork;
    MonitorInfo.dwFlags = 0;

    if (Monitor->IsPrimary)
        MonitorInfo.dwFlags |= MONITORINFOF_PRIMARY;

    /* fill device name */
    if (MonitorInfo.cbSize == sizeof (MONITORINFOEXW))
    {
        WCHAR nul = L'\0';
        INT len = Monitor->DeviceName.Length;
        if (len >= CCHDEVICENAME * sizeof (WCHAR))
            len = (CCHDEVICENAME - 1) * sizeof (WCHAR);

        memcpy(MonitorInfo.szDevice, Monitor->DeviceName.Buffer, len);
        memcpy(MonitorInfo.szDevice + (len / sizeof (WCHAR)), &nul, sizeof (WCHAR));
    }

    /* output data */
    Status = MmCopyToCaller(pMonitorInfo, &MonitorInfo, MonitorInfo.cbSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("GetMonitorInfo: MmCopyToCaller failed\n");
        SetLastNtError(Status);
        RETURN(FALSE);
    }

    DPRINT("GetMonitorInfo: success\n");

    RETURN(TRUE);

CLEANUP:
    DPRINT("Leave NtUserGetMonitorInfo, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

/* NtUserMonitorFromPoint
 *
 * Returns a handle to the monitor containing the given point.
 *
 * Arguments
 *
 *   point
 *     Point for which to find monitor
 *
 *   dwFlags
 *     Specifies the behaviour if the point isn't on any of the monitors.
 *
 * Return value
 *   If the point is found a handle to the monitor is returned; if not the
 *   return value depends on dwFlags
 */
HMONITOR
APIENTRY
NtUserMonitorFromPoint(
    IN POINT point,
    IN DWORD dwFlags)
{
    INT NumMonitors;
    RECTL InRect;
    HMONITOR hMonitor = NULL;

    /* fill inRect (bottom-right exclusive) */
    InRect.left = point.x;
    InRect.right = point.x + 1;
    InRect.top = point.y;
    InRect.bottom = point.y + 1;

    /* find intersecting monitor */
    NumMonitors = IntGetMonitorsFromRect(&InRect, &hMonitor, NULL, 1, dwFlags);
    if (NumMonitors < 0)
    {
        return (HMONITOR)NULL;
    }

    return hMonitor;
}

/* NtUserMonitorFromRect
 *
 * Returns a handle to the monitor having the largest intersection with a
 * given rectangle
 *
 * Arguments
 *
 *   pRect
 *     Pointer to a RECT for which to find monitor
 *
 *   dwFlags
 *     Specifies the behaviour if no monitor intersects the given rect
 *
 * Return value
 *   If a monitor intersects the rect a handle to it is returned; if not the
 *   return value depends on dwFlags
 */
HMONITOR
APIENTRY
NtUserMonitorFromRect(
    IN LPCRECTL pRect,
    IN DWORD dwFlags)
{
    ULONG numMonitors, iLargestArea = 0, i;
    PRECTL rectList;
    HMONITOR *hMonitorList;
    HMONITOR hMonitor = NULL;
    RECTL rect;
    NTSTATUS status;

    /* get rect */
    status = MmCopyFromCaller(&rect, pRect, sizeof (RECT));
    if (!NT_SUCCESS(status))
    {
        SetLastNtError(status);
        return (HMONITOR)NULL;
    }

    /* find intersecting monitors */
    numMonitors = IntGetMonitorsFromRect(&rect, &hMonitor, NULL, 1, dwFlags);
    if (numMonitors <= 1)
    {
        return hMonitor;
    }

    hMonitorList = ExAllocatePoolWithTag(PagedPool, 
                                         sizeof(HMONITOR) * numMonitors,
                                         USERTAG_MONITORRECTS);
    if (hMonitorList == NULL)
    {
        /* FIXME: EngSetLastError? */
        return (HMONITOR)NULL;
    }

    rectList = ExAllocatePoolWithTag(PagedPool,
                                     sizeof(RECT) * numMonitors,
                                     USERTAG_MONITORRECTS);
    if (rectList == NULL)
    {
        ExFreePoolWithTag(hMonitorList, USERTAG_MONITORRECTS);
        /* FIXME: EngSetLastError? */
        return (HMONITOR)NULL;
    }

    /* get intersecting monitors */
    numMonitors = IntGetMonitorsFromRect(&rect, hMonitorList, rectList,
                                         numMonitors, 0);
    if (numMonitors == 0)
    {
        ExFreePoolWithTag(hMonitorList, USERTAG_MONITORRECTS);
        ExFreePoolWithTag(rectList, USERTAG_MONITORRECTS);
        return (HMONITOR)NULL;
    }

    /* find largest intersection */
    for (i = 0; i < numMonitors; i++)
    {
        ULONG area = (rectList[i].right - rectList[i].left) *
                     (rectList[i].bottom - rectList[i].top);
        if (area >= iLargestArea)
        {
            hMonitor = hMonitorList[i];
        }
    }

    ExFreePoolWithTag(hMonitorList, USERTAG_MONITORRECTS);
    ExFreePoolWithTag(rectList, USERTAG_MONITORRECTS);

    return hMonitor;
}


HMONITOR
APIENTRY
NtUserMonitorFromWindow(
    IN HWND hWnd,
    IN DWORD dwFlags)
{
    PWND Window;
    HMONITOR hMonitor = NULL;
    RECTL Rect;
    DECLARE_RETURN(HMONITOR);

    DPRINT("Enter NtUserMonitorFromWindow\n");
    UserEnterShared();

    if (!(Window = UserGetWindowObject(hWnd)))
    {
        if (dwFlags == MONITOR_DEFAULTTONULL)
        {
            RETURN(hMonitor);
        }
        IntGetMonitorsFromRect(NULL, &hMonitor, NULL, 1, dwFlags);
        RETURN(hMonitor);
    }

    Rect.left = Rect.right = Window->rcWindow.left;
    Rect.top = Rect.bottom = Window->rcWindow.bottom;

    IntGetMonitorsFromRect(&Rect, &hMonitor, NULL, 1, dwFlags);

    RETURN(hMonitor);

CLEANUP:
    DPRINT("Leave NtUserMonitorFromWindow, ret=%i\n",_ret_);
    UserLeave();
    END_CLEANUP;
}
