/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          pMonitor support
 *  FILE:             subsys/win32k/ntuser/monitor.c
 *  PROGRAMERS:       Anich Gregor (blight@blight.eu.org)
 *                    Rafal Harabien (rafalh@reactos.org)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMonitor);

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* List of monitors */
static PMONITOR gMonitorList = NULL;

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
    return UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_MONITOR, sizeof(MONITOR));
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
    /* Remove monitor region */
    if (pMonitor->hrgnMonitor)
    {
        GreSetObjectOwner(pMonitor->hrgnMonitor, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(pMonitor->hrgnMonitor);
    }

    /* Destroy monitor object */
    UserDereferenceObject(pMonitor);
    UserDeleteObject(UserHMGetHandle(pMonitor), TYPE_MONITOR);
}

/* UserGetMonitorObject
 *
 * Returns monitor object from handle or sets last error if handle is invalid
 *
 * Arguments
 *
 *   hMonitor
 *      Handle of MONITOR object
 */
PMONITOR NTAPI
UserGetMonitorObject(IN HMONITOR hMonitor)
{
    PMONITOR pMonitor;

    if (!hMonitor)
    {
        EngSetLastError(ERROR_INVALID_MONITOR_HANDLE);
        return NULL;
    }

    pMonitor = (PMONITOR)UserGetObject(gHandleTable, hMonitor, TYPE_MONITOR);
    if (!pMonitor)
    {
        EngSetLastError(ERROR_INVALID_MONITOR_HANDLE);
        return NULL;
    }

    return pMonitor;
}

/* UserGetPrimaryMonitor
 *
 * Returns a PMONITOR for the primary monitor
 *
 * Return value
 *   PMONITOR
 */
PMONITOR NTAPI
UserGetPrimaryMonitor()
{
    PMONITOR pMonitor;

    /* Find primary monitor */
    for (pMonitor = gMonitorList; pMonitor != NULL; pMonitor = pMonitor->pMonitorNext)
    {
        if (pMonitor->IsPrimary)
            break;
    }

    return pMonitor;
}

/* UserAttachMonitor
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
NTSTATUS NTAPI
UserAttachMonitor(IN HDEV hDev)
{
    PMONITOR pMonitor;

    TRACE("Attaching monitor...\n");

    /* Create new monitor object */
    pMonitor = IntCreateMonitorObject();
    if (pMonitor == NULL)
    {
        TRACE("Couldnt create monitor object\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pMonitor->hDev = hDev;
    pMonitor->cWndStack = 0;

    if (gMonitorList == NULL)
    {
        TRACE("Primary monitor is beeing attached\n");
        pMonitor->IsPrimary = TRUE;
        gMonitorList = pMonitor;
    }
    else
    {
        PMONITOR pmonLast = gMonitorList;
        TRACE("Additional monitor is beeing attached\n");
        while (pmonLast->pMonitorNext != NULL)
            pmonLast = pmonLast->pMonitorNext;

        pmonLast->pMonitorNext = pMonitor;
    }

    UserUpdateMonitorSize(hDev);

    return STATUS_SUCCESS;
}

/* UserDetachMonitor
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
NTSTATUS NTAPI
UserDetachMonitor(IN HDEV hDev)
{
    PMONITOR pMonitor = gMonitorList, *pLink = &gMonitorList;

    /* Find monitor attached to given device */
    while (pMonitor != NULL)
    {
        if (pMonitor->hDev == hDev)
            break;

        pLink = &pMonitor->pMonitorNext;
        pMonitor = pMonitor->pMonitorNext;
    }

    if (pMonitor == NULL)
    {
        /* No monitor has been found */
        return STATUS_INVALID_PARAMETER;
    }

    /* We destroy primary monitor - set next as primary */
    if (pMonitor->IsPrimary && pMonitor->pMonitorNext != NULL)
        pMonitor->pMonitorNext->IsPrimary = TRUE;

    /* Update Next ptr in previous monitor */
    *pLink = pMonitor->pMonitorNext;

    /* Finally destroy monitor */
    IntDestroyMonitorObject(pMonitor);

    return STATUS_SUCCESS;
}

/* UserUpdateMonitorSize
 *
 * Reset size of the monitor using atached device
 *
 * Arguments
 *
 *   PMONITOR
 *      pGdiDevice  Pointer to the PDEVOBJ, which size has changed
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS NTAPI
UserUpdateMonitorSize(IN HDEV hDev)
{
	PMONITOR pMonitor;
    SIZEL DeviceSize;

    /* Find monitor attached to given device */
    for (pMonitor = gMonitorList; pMonitor != NULL; pMonitor = pMonitor->pMonitorNext)
    {
        if (pMonitor->hDev == hDev)
            break;
    }

    if (pMonitor == NULL)
    {
        /* No monitor has been found */
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the size of the hdev */
    PDEVOBJ_sizl((PPDEVOBJ)hDev, &DeviceSize);

    /* Update monitor size */
    pMonitor->rcMonitor.left  = 0;
    pMonitor->rcMonitor.top   = 0;
    pMonitor->rcMonitor.right  = pMonitor->rcMonitor.left + DeviceSize.cx;
    pMonitor->rcMonitor.bottom = pMonitor->rcMonitor.top + DeviceSize.cy;
    pMonitor->rcWork = pMonitor->rcMonitor;

    /* Destroy monitor region... */
    if (pMonitor->hrgnMonitor)
    {
        GreSetObjectOwner(pMonitor->hrgnMonitor, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(pMonitor->hrgnMonitor);
    }

    /* ...and create new one */
    pMonitor->hrgnMonitor = NtGdiCreateRectRgn(
        pMonitor->rcMonitor.left,
        pMonitor->rcMonitor.top,
        pMonitor->rcMonitor.right,
        pMonitor->rcMonitor.bottom);
    if (pMonitor->hrgnMonitor)
        IntGdiSetRegionOwner(pMonitor->hrgnMonitor, GDI_OBJ_HMGR_PUBLIC);

    return STATUS_SUCCESS;
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
 *   phMonitorList
 *      Pointer to an array of HMONITOR which is filled with monitor handles.
 *      Can be NULL
 *
 *   prcMonitorList
 *      Pointer to an array of RECT which is filled with intersection rects in
 *      desktop coordinates.
 *      Can be NULL, will be ignored if no intersecting monitor is found and
 *      flags is MONITOR_DEFAULTTONEAREST
 *
 *   dwListSize
 *      Size of the phMonitorList and prcMonitorList arguments. If this is zero
 *      phMonitorList and prcMonitorList are ignored.
 *
 *   dwFlags
 *      Either 0 or MONITOR_DEFAULTTONEAREST (ignored if rect is NULL)
 *
 * Returns
 *   The number of monitors which intersect the specified region.
 */
static
UINT
IntGetMonitorsFromRect(OPTIONAL IN LPCRECTL pRect,
                       OPTIONAL OUT HMONITOR *phMonitorList,
                       OPTIONAL OUT PRECTL prcMonitorList,
                       OPTIONAL IN DWORD dwListSize,
                       OPTIONAL IN DWORD dwFlags)
{
    PMONITOR pMonitor, pNearestMonitor = NULL, pPrimaryMonitor = NULL;
    UINT cMonitors = 0;
    ULONG iNearestDistance = 0xffffffff;

    /* Find monitors which intersects the rectangle */
    for (pMonitor = gMonitorList; pMonitor != NULL; pMonitor = pMonitor->pMonitorNext)
    {
        RECTL MonitorRect, IntersectionRect;

        MonitorRect = pMonitor->rcMonitor;

        TRACE("MonitorRect: left = %d, top = %d, right = %d, bottom = %d\n",
               MonitorRect.left, MonitorRect.top, MonitorRect.right, MonitorRect.bottom);

        /* Save primary monitor for later usage */
        if (dwFlags == MONITOR_DEFAULTTOPRIMARY && pMonitor->IsPrimary)
            pPrimaryMonitor = pMonitor;

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
            if (dwFlags == MONITOR_DEFAULTTONEAREST)
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
                    pNearestMonitor = pMonitor;
                }
            }

            continue;
        }

        /* Check if there's space in the buffer */
        if (cMonitors < dwListSize)
        {
            /* Save monitor data */
            if (phMonitorList != NULL)
                phMonitorList[cMonitors] = UserHMGetHandle(pMonitor);
            if (prcMonitorList != NULL)
                prcMonitorList[cMonitors] = IntersectionRect;
        }

        /* Increase count of found monitors */
        cMonitors++;
    }

    /* Nothing has been found? */
    if (cMonitors == 0)
    {
        /* Check if we shall default to the nearest monitor */
        if (dwFlags == MONITOR_DEFAULTTONEAREST && pNearestMonitor)
        {
            if (phMonitorList && dwListSize > 0)
                phMonitorList[cMonitors] = UserHMGetHandle(pNearestMonitor);
            cMonitors++;
        }
        /* Check if we shall default to the primary monitor */
        else if (dwFlags == MONITOR_DEFAULTTOPRIMARY && pPrimaryMonitor)
        {
            if (phMonitorList != NULL && dwListSize > 0)
                phMonitorList[cMonitors] = UserHMGetHandle(pPrimaryMonitor);
            cMonitors++;
        }
    }

    return cMonitors;
}

PMONITOR NTAPI
UserMonitorFromRect(
    PRECTL pRect,
    DWORD dwFlags)
{
    ULONG cMonitors, LargestArea = 0, i;
    PRECTL prcMonitorList = NULL;
    HMONITOR *phMonitorList = NULL;
    HMONITOR hMonitor = NULL;

    /* Check if flags are valid */
    if (dwFlags != MONITOR_DEFAULTTONULL &&
        dwFlags != MONITOR_DEFAULTTOPRIMARY &&
        dwFlags != MONITOR_DEFAULTTONEAREST)
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    /* Find intersecting monitors */
    cMonitors = IntGetMonitorsFromRect(pRect, &hMonitor, NULL, 1, dwFlags);
    if (cMonitors <= 1)
    {
        /* No or one monitor found. Just return handle. */
        goto cleanup;
    }

    /* There is more than one monitor. Find monitor with largest intersection.
       Temporary reset hMonitor */
    hMonitor = NULL;

    /* Allocate helper buffers */
    phMonitorList = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(HMONITOR) * cMonitors,
                                          USERTAG_MONITORRECTS);
    if (phMonitorList == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    prcMonitorList = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(RECT) * cMonitors,
                                           USERTAG_MONITORRECTS);
    if (prcMonitorList == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    /* Get intersecting monitors again but now with rectangle list */
    cMonitors = IntGetMonitorsFromRect(pRect, phMonitorList, prcMonitorList,
                                       cMonitors, 0);

    /* Find largest intersection */
    for (i = 0; i < cMonitors; i++)
    {
        ULONG Area = (prcMonitorList[i].right - prcMonitorList[i].left) *
                     (prcMonitorList[i].bottom - prcMonitorList[i].top);
        if (Area >= LargestArea)
        {
            hMonitor = phMonitorList[i];
            LargestArea = Area;
        }
    }

cleanup:
    if (phMonitorList)
        ExFreePoolWithTag(phMonitorList, USERTAG_MONITORRECTS);
    if (prcMonitorList)
        ExFreePoolWithTag(prcMonitorList, USERTAG_MONITORRECTS);

    return UserGetMonitorObject(hMonitor);
}

PMONITOR
FASTCALL
UserMonitorFromPoint(
    IN POINT pt,
    IN DWORD dwFlags)
{
    RECTL rc;
    HMONITOR hMonitor = NULL;

    /* Check if flags are valid */
    if (dwFlags != MONITOR_DEFAULTTONULL &&
        dwFlags != MONITOR_DEFAULTTOPRIMARY &&
        dwFlags != MONITOR_DEFAULTTONEAREST)
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    /* Fill rect (bottom-right exclusive) */
    rc.left = pt.x;
    rc.right = pt.x + 1;
    rc.top = pt.y;
    rc.bottom = pt.y + 1;

    /* Find intersecting monitor */
    IntGetMonitorsFromRect(&rc, &hMonitor, NULL, 1, dwFlags);

    return UserGetMonitorObject(hMonitor);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/* NtUserEnumDisplayMonitors
 *
 * Enumerates display monitors which intersect the given HDC/cliprect
 *
 * Arguments
 *
 *   hdc
 *      Handle to a DC for which to enum intersecting monitors. If this is NULL
 *      it returns all monitors which are part of the current virtual screen.
 *
 *   pUnsafeRect
 *      Clipping rectangle with coordinate system origin at the DCs origin if the
 *      given HDC is not NULL or in virtual screen coordinated if it is NULL.
 *      Can be NULL
 *
 *   phUnsafeMonitorList
 *      Pointer to an array of HMONITOR which is filled with monitor handles.
 *      Can be NULL
 *
 *   prcUnsafeMonitorList
 *      Pointer to an array of RECT which is filled with intersection rectangles.
 *      Can be NULL
 *
 *   dwListSize
 *      Size of the hMonitorList and monitorRectList arguments. If this is zero
 *      hMonitorList and monitorRectList are ignored.
 *
 * Returns
 *   The number of monitors which intersect the specified region or -1 on failure.
 */
INT
APIENTRY
NtUserEnumDisplayMonitors(
    OPTIONAL IN HDC hdc,
    OPTIONAL IN LPCRECTL pUnsafeRect,
    OPTIONAL OUT HMONITOR *phUnsafeMonitorList,
    OPTIONAL OUT PRECTL prcUnsafeMonitorList,
    OPTIONAL IN DWORD dwListSize)
{
    UINT cMonitors, i;
    INT iRet = -1;
    HMONITOR *phMonitorList = NULL;
    PRECTL prcMonitorList = NULL;
    RECTL rc, *pRect;
    RECTL DcRect = {0};
    NTSTATUS Status;

    /* Get rectangle */
    if (pUnsafeRect != NULL)
    {
        Status = MmCopyFromCaller(&rc, pUnsafeRect, sizeof(RECT));
        if (!NT_SUCCESS(Status))
        {
            TRACE("MmCopyFromCaller() failed!\n");
            SetLastNtError(Status);
            return -1;
        }
    }

    if (hdc != NULL)
    {
        PDC pDc;
        INT iRgnType;

        /* Get visible region bounding rect */
        pDc = DC_LockDc(hdc);
        if (pDc == NULL)
        {
            TRACE("DC_LockDc() failed!\n");
            /* FIXME: setlasterror? */
            return -1;
        }
        iRgnType = REGION_GetRgnBox(pDc->prgnVis, &DcRect);
        DC_UnlockDc(pDc);

        if (iRgnType == 0)
        {
            TRACE("NtGdiGetRgnBox() failed!\n");
            return -1;
        }
        if (iRgnType == NULLREGION)
            return 0;
        if (iRgnType == COMPLEXREGION)
        {
            /* TODO: Warning */
        }

        /* If hdc and pRect are given the area of interest is pRect with
           coordinate origin at the DC position */
        if (pUnsafeRect != NULL)
        {
            rc.left += DcRect.left;
            rc.right += DcRect.left;
            rc.top += DcRect.top;
            rc.bottom += DcRect.top;
        }
        /* If hdc is given and pRect is not the area of interest is the
           bounding rect of hdc */
        else
        {
            rc = DcRect;
        }
    }

    if (hdc == NULL && pUnsafeRect == NULL)
        pRect = NULL;
    else
        pRect = &rc;

    UserEnterShared();

    /* Find intersecting monitors */
    cMonitors = IntGetMonitorsFromRect(pRect, NULL, NULL, 0, MONITOR_DEFAULTTONULL);
    if (cMonitors == 0 || dwListSize == 0 ||
        (phUnsafeMonitorList == NULL && prcUnsafeMonitorList == NULL))
    {
        /* Simple case - just return monitors count */
        TRACE("cMonitors = %u\n", cMonitors);
        iRet = cMonitors;
        goto cleanup;
    }

    /* Allocate safe buffers */
    if (phUnsafeMonitorList != NULL && dwListSize != 0)
    {
        phMonitorList = ExAllocatePoolWithTag(PagedPool, sizeof (HMONITOR) * dwListSize, USERTAG_MONITORRECTS);
        if (phMonitorList == NULL)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }
    if (prcUnsafeMonitorList != NULL && dwListSize != 0)
    {
        prcMonitorList = ExAllocatePoolWithTag(PagedPool, sizeof(RECT) * dwListSize,USERTAG_MONITORRECTS);
        if (prcMonitorList == NULL)
        {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }

    /* Get intersecting monitors */
    cMonitors = IntGetMonitorsFromRect(pRect, phMonitorList, prcMonitorList,
                                       dwListSize, MONITOR_DEFAULTTONULL);

    if (hdc != NULL && pRect != NULL && prcMonitorList != NULL)
    {
        for (i = 0; i < min(cMonitors, dwListSize); i++)
        {
            _Analysis_assume_(i < dwListSize);
            prcMonitorList[i].left -= DcRect.left;
            prcMonitorList[i].right -= DcRect.left;
            prcMonitorList[i].top -= DcRect.top;
            prcMonitorList[i].bottom -= DcRect.top;
        }
    }

    /* Output result */
    if (phUnsafeMonitorList != NULL && dwListSize != 0)
    {
        Status = MmCopyToCaller(phUnsafeMonitorList, phMonitorList, sizeof(HMONITOR) * dwListSize);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto cleanup;
        }
    }
    if (prcUnsafeMonitorList != NULL && dwListSize != 0)
    {
        Status = MmCopyToCaller(prcUnsafeMonitorList, prcMonitorList, sizeof(RECT) * dwListSize);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            goto cleanup;
        }
    }

    /* Return monitors count on success */
    iRet = cMonitors;

cleanup:
    if (phMonitorList)
        ExFreePoolWithTag(phMonitorList, USERTAG_MONITORRECTS);
    if (prcMonitorList)
        ExFreePoolWithTag(prcMonitorList, USERTAG_MONITORRECTS);

    UserLeave();
    return iRet;
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
 *   pMonitorInfoUnsafe
 *      Pointer to a MONITORINFO struct which is filled with the information.
 *      The cbSize member must be set to sizeof(MONITORINFO) or
 *      sizeof(MONITORINFOEX). Even if set to sizeof(MONITORINFOEX) only parts
 *      from MONITORINFO will be filled.
 *
 *   pDevice
 *      Pointer to a UNICODE_STRING which will receive the device's name. The
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
    OUT LPMONITORINFO pMonitorInfoUnsafe)
{
    PMONITOR pMonitor;
    MONITORINFOEXW MonitorInfo;
    NTSTATUS Status;
    BOOL bRet = FALSE;
    PWCHAR pwstrDeviceName;

    TRACE("Enter NtUserGetMonitorInfo\n");
    UserEnterShared();

    /* Get monitor object */
    pMonitor = UserGetMonitorObject(hMonitor);
    if (!pMonitor)
    {
        TRACE("Couldnt find monitor %p\n", hMonitor);
        goto cleanup;
    }

    /* Check if pMonitorInfoUnsafe is valid */
    if(pMonitorInfoUnsafe == NULL)
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    pwstrDeviceName = ((PPDEVOBJ)(pMonitor->hDev))->pGraphicsDevice->szWinDeviceName;

    /* Get size of pMonitorInfoUnsafe */
    Status = MmCopyFromCaller(&MonitorInfo.cbSize, &pMonitorInfoUnsafe->cbSize, sizeof(MonitorInfo.cbSize));
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        goto cleanup;
    }

    /* Check if size of struct is valid */
    if (MonitorInfo.cbSize != sizeof(MONITORINFO) &&
        MonitorInfo.cbSize != sizeof(MONITORINFOEXW))
    {
        SetLastNtError(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    /* Fill monitor info */
    MonitorInfo.rcMonitor = pMonitor->rcMonitor;
    MonitorInfo.rcWork = pMonitor->rcWork;
    MonitorInfo.dwFlags = 0;
    if (pMonitor->IsPrimary)
        MonitorInfo.dwFlags |= MONITORINFOF_PRIMARY;

    /* Fill device name */
    if (MonitorInfo.cbSize == sizeof(MONITORINFOEXW))
    {
        RtlStringCbCopyNExW(MonitorInfo.szDevice,
                          sizeof(MonitorInfo.szDevice),
                          pwstrDeviceName,
                          (wcslen(pwstrDeviceName)+1) * sizeof(WCHAR),
                          NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
    }

    /* Output data */
    Status = MmCopyToCaller(pMonitorInfoUnsafe, &MonitorInfo, MonitorInfo.cbSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("GetMonitorInfo: MmCopyToCaller failed\n");
        SetLastNtError(Status);
        goto cleanup;
    }

    TRACE("GetMonitorInfo: success\n");
    bRet = TRUE;

cleanup:
    TRACE("Leave NtUserGetMonitorInfo, ret=%i\n", bRet);
    UserLeave();
    return bRet;
}

/* NtUserMonitorFromPoint
 *
 * Returns a handle to the monitor containing the given point.
 *
 * Arguments
 *
 *   pt
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
    IN POINT pt,
    IN DWORD dwFlags)
{
    RECTL rc;
    HMONITOR hMonitor = NULL;

    /* Check if flags are valid */
    if (dwFlags != MONITOR_DEFAULTTONULL &&
        dwFlags != MONITOR_DEFAULTTOPRIMARY &&
        dwFlags != MONITOR_DEFAULTTONEAREST)
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    /* Fill rect (bottom-right exclusive) */
    rc.left = pt.x;
    rc.right = pt.x + 1;
    rc.top = pt.y;
    rc.bottom = pt.y + 1;

    UserEnterShared();

    /* Find intersecting monitor */
    IntGetMonitorsFromRect(&rc, &hMonitor, NULL, 1, dwFlags);

    UserLeave();
    return hMonitor;
}

/* NtUserMonitorFromRect
 *
 * Returns a handle to the monitor having the largest intersection with a
 * given rectangle
 *
 * Arguments
 *
 *   pRectUnsafe
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
    IN LPCRECTL pRectUnsafe,
    IN DWORD dwFlags)
{
    ULONG cMonitors, LargestArea = 0, i;
    PRECTL prcMonitorList = NULL;
    HMONITOR *phMonitorList = NULL;
    HMONITOR hMonitor = NULL;
    RECTL Rect;
    NTSTATUS Status;

    /* Check if flags are valid */
    if (dwFlags != MONITOR_DEFAULTTONULL &&
        dwFlags != MONITOR_DEFAULTTOPRIMARY &&
        dwFlags != MONITOR_DEFAULTTONEAREST)
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    /* Copy rectangle to safe buffer */
    Status = MmCopyFromCaller(&Rect, pRectUnsafe, sizeof (RECT));
    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    UserEnterShared();

    /* Find intersecting monitors */
    cMonitors = IntGetMonitorsFromRect(&Rect, &hMonitor, NULL, 1, dwFlags);
    if (cMonitors <= 1)
    {
        /* No or one monitor found. Just return handle. */
        goto cleanup;
    }

    /* There is more than one monitor. Find monitor with largest intersection.
       Temporary reset hMonitor */
    hMonitor = NULL;

    /* Allocate helper buffers */
    phMonitorList = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(HMONITOR) * cMonitors,
                                          USERTAG_MONITORRECTS);
    if (phMonitorList == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    prcMonitorList = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(RECT) * cMonitors,
                                           USERTAG_MONITORRECTS);
    if (prcMonitorList == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    /* Get intersecting monitors again but now with rectangle list */
    cMonitors = IntGetMonitorsFromRect(&Rect, phMonitorList, prcMonitorList,
                                       cMonitors, 0);

    /* Find largest intersection */
    for (i = 0; i < cMonitors; i++)
    {
        ULONG Area = (prcMonitorList[i].right - prcMonitorList[i].left) *
                     (prcMonitorList[i].bottom - prcMonitorList[i].top);
        if (Area >= LargestArea)
        {
            hMonitor = phMonitorList[i];
            LargestArea = Area;
        }
    }

cleanup:
    if (phMonitorList)
        ExFreePoolWithTag(phMonitorList, USERTAG_MONITORRECTS);
    if (prcMonitorList)
        ExFreePoolWithTag(prcMonitorList, USERTAG_MONITORRECTS);
    UserLeave();

    return hMonitor;
}


HMONITOR
APIENTRY
NtUserMonitorFromWindow(
    IN HWND hWnd,
    IN DWORD dwFlags)
{
    PWND pWnd;
    HMONITOR hMonitor = NULL;
    RECTL Rect = {0, 0, 0, 0};

    TRACE("Enter NtUserMonitorFromWindow\n");

    /* Check if flags are valid */
    if (dwFlags != MONITOR_DEFAULTTONULL &&
        dwFlags != MONITOR_DEFAULTTOPRIMARY &&
        dwFlags != MONITOR_DEFAULTTONEAREST)
    {
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    UserEnterShared();

    /* If window is given, use it first */
    if (hWnd)
    {
        /* Get window object */
        pWnd = UserGetWindowObject(hWnd);
        if (!pWnd)
            goto cleanup;

        /* Find only monitors which have intersection with given window */
        Rect.left = Rect.right = pWnd->rcWindow.left;
        Rect.top = Rect.bottom = pWnd->rcWindow.bottom;
    }

    /* Find monitors now */
    IntGetMonitorsFromRect(&Rect, &hMonitor, NULL, 1, dwFlags);

cleanup:
    TRACE("Leave NtUserMonitorFromWindow, ret=%p\n", hMonitor);
    UserLeave();
    return hMonitor;
}
