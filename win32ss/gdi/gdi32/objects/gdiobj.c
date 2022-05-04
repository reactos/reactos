#include <precomp.h>

#define NDEBUG
#include <debug.h>

HGDIOBJ stock_objects[NB_STOCK_OBJECTS];

/*
 * @implemented
 */
HGDIOBJ
WINAPI
GetStockObject(
    INT fnObject)
{
    HGDIOBJ hobj;

    if ((fnObject < 0) || (fnObject >= NB_STOCK_OBJECTS))
        return NULL;

    hobj = stock_objects[fnObject];
    if (hobj == NULL)
    {
        hobj = NtGdiGetStockObject(fnObject);

        if (!GdiValidateHandle(hobj))
        {
            return NULL;
        }

        stock_objects[fnObject] = hobj;
    }

    return hobj;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetObjectType(
    HGDIOBJ h)
{
    DWORD Ret = 0;

    if (GdiValidateHandle(h))
    {
        LONG Type = GDI_HANDLE_GET_TYPE(h);
        switch(Type)
        {
        case GDI_OBJECT_TYPE_PEN:
            Ret = OBJ_PEN;
            break;
        case GDI_OBJECT_TYPE_BRUSH:
            Ret = OBJ_BRUSH;
            break;
        case GDI_OBJECT_TYPE_BITMAP:
            Ret = OBJ_BITMAP;
            break;
        case GDI_OBJECT_TYPE_FONT:
            Ret = OBJ_FONT;
            break;
        case GDI_OBJECT_TYPE_PALETTE:
            Ret = OBJ_PAL;
            break;
        case GDI_OBJECT_TYPE_REGION:
            Ret = OBJ_REGION;
            break;
        case GDI_OBJECT_TYPE_DC:
            if ( GetDCDWord( h, GdiGetIsMemDc, 0))
            {
                Ret = OBJ_MEMDC;
            }
            else
                Ret = OBJ_DC;
            break;
        case GDI_OBJECT_TYPE_COLORSPACE:
            Ret = OBJ_COLORSPACE;
            break;
        case GDI_OBJECT_TYPE_METAFILE:
            Ret = OBJ_METAFILE;
            break;
        case GDI_OBJECT_TYPE_ENHMETAFILE:
            Ret = OBJ_ENHMETAFILE;
            break;
        case GDI_OBJECT_TYPE_METADC:
            Ret = OBJ_METADC;
            break;
        case GDI_OBJECT_TYPE_EXTPEN:
            Ret = OBJ_EXTPEN;
            break;

        case GDILoObjType_LO_ALTDC_TYPE:
            // FIXME: could be something else?
            Ret = OBJ_ENHMETADC;
            break;

        default:
            DPRINT1("GetObjectType: Magic 0x%08x not implemented\n", Type);
            break;
        }
    }
    else
        /* From Wine: GetObjectType does SetLastError() on a null object */
        SetLastError(ERROR_INVALID_HANDLE);
    return Ret;
}

ULONG
WINAPI
GetFontObjectA(
    _In_ HGDIOBJ hfont,
    _In_ ULONG cbSize,
    _Out_ LPVOID lpBuffer)
{
    ENUMLOGFONTEXDVW elfedvW;
    ENUMLOGFONTEXDVA elfedvA;
    ULONG cbResult;

    /* Check if size only is requested */
    if (!lpBuffer) return sizeof(LOGFONTA);

    /* Check for size 0 */
    if (cbSize == 0)
    {
        /* Windows does not SetLastError() */
        return 0;
    }

    /* Windows does this ... */
    if (cbSize == sizeof(LOGFONTW)) cbSize = sizeof(LOGFONTA);

    /* Call win32k to get the logfont (widechar) */
    cbResult = NtGdiExtGetObjectW(hfont, sizeof(ENUMLOGFONTEXDVW), &elfedvW);
    if (cbResult == 0)
    {
        return 0;
    }

    /* Convert the logfont from widechar to ansi */
    EnumLogFontExW2A(&elfedvA.elfEnumLogfontEx, &elfedvW.elfEnumLogfontEx);
    elfedvA.elfDesignVector = elfedvW.elfDesignVector;

    /* Don't copy more than maximum */
    if (cbSize > sizeof(ENUMLOGFONTEXDVA)) cbSize = sizeof(ENUMLOGFONTEXDVA);

    /* Copy the number of bytes requested */
    memcpy(lpBuffer, &elfedvA, cbSize);

    /* Return the number of bytes copied */
    return cbSize;
}


/*
 * @implemented
 */
int
WINAPI
GetObjectA(
    _In_ HGDIOBJ hGdiObj,
    _In_ int cbSize,
    _Out_ LPVOID lpBuffer)
{
    DWORD dwType = GDI_HANDLE_GET_TYPE(hGdiObj);

    /* Chjeck if this is anything else but a font */
    if (dwType == GDI_OBJECT_TYPE_FONT)
    {
        return GetFontObjectA(hGdiObj, cbSize, lpBuffer);
    }
    else
    {
        /* Simply pass it to the widechar version */
        return GetObjectW(hGdiObj, cbSize, lpBuffer);
    }
}


/*
 * @implemented
 */
int
WINAPI
GetObjectW(
    _In_ HGDIOBJ hGdiObj,
    _In_ int cbSize,
    _Out_ LPVOID lpBuffer)
{
    DWORD dwType;
    INT cbResult = 0;

    /* Fixup handles with upper 16 bits masked */
    hGdiObj = GdiFixUpHandle(hGdiObj);

    /* Get the object type */
    dwType = GDI_HANDLE_GET_TYPE(hGdiObj);

    /* Check what kind of object we have */
    switch (dwType)
    {
        case GDI_OBJECT_TYPE_PEN:
            if (!lpBuffer) return sizeof(LOGPEN);
            break;

        case GDI_OBJECT_TYPE_BRUSH:
            if (!lpBuffer) return sizeof(LOGBRUSH);
            break;

        case GDI_OBJECT_TYPE_BITMAP:
            if (!lpBuffer) return sizeof(BITMAP);
            break;

        case GDI_OBJECT_TYPE_PALETTE:
            if (!lpBuffer) return sizeof(WORD);
            break;

        case GDI_OBJECT_TYPE_FONT:
            if (!lpBuffer) return sizeof(LOGFONTW);
            break;

        case GDI_OBJECT_TYPE_EXTPEN:
            /* we don't know the size, ask win32k */
            break;

        case GDI_OBJECT_TYPE_COLORSPACE:
            if ((cbSize < 328) || !lpBuffer)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }
            break;

        case GDI_OBJECT_TYPE_DC:
        case GDI_OBJECT_TYPE_REGION:
        case GDI_OBJECT_TYPE_EMF:
        case GDI_OBJECT_TYPE_METAFILE:
        case GDI_OBJECT_TYPE_ENHMETAFILE:
            SetLastError(ERROR_INVALID_HANDLE);
        default:
            return 0;
    }

    /* Call win32k */
    cbResult = NtGdiExtGetObjectW(hGdiObj, cbSize, lpBuffer);

    /* Handle error */
    if (cbResult == 0)
    {
        if (!GdiValidateHandle(hGdiObj))
        {
            if ((dwType == GDI_OBJECT_TYPE_PEN) ||
                (dwType == GDI_OBJECT_TYPE_EXTPEN) ||
                (dwType == GDI_OBJECT_TYPE_BRUSH) ||
                (dwType == GDI_OBJECT_TYPE_COLORSPACE))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
        else
        {
            if ((dwType == GDI_OBJECT_TYPE_PEN) ||
                (dwType == GDI_OBJECT_TYPE_BRUSH) ||
                (dwType == GDI_OBJECT_TYPE_COLORSPACE) ||
                ( (dwType == GDI_OBJECT_TYPE_EXTPEN) &&
                    ( (cbSize >= sizeof(EXTLOGPEN)) || (cbSize == 0) ) ) ||
                ( (dwType == GDI_OBJECT_TYPE_BITMAP) && (cbSize >= sizeof(BITMAP)) ))
            {
                SetLastError(ERROR_NOACCESS);
            }
        }
    }

    return cbResult;
}

static
BOOL
GdiDeleteBrushOrPen(
    HGDIOBJ hobj)
{
    GDILOOBJTYPE eObjectType;
    PBRUSH_ATTR pbrattr;
    PTEB pTeb;
    PGDIBSOBJECT pgO;

    eObjectType = GDI_HANDLE_GET_TYPE(hobj);

    if ((GdiGetHandleUserData(hobj, eObjectType, (PVOID*)&pbrattr)) &&
        (pbrattr != NULL))
    {
        pTeb = NtCurrentTeb();
        if (pTeb->Win32ThreadInfo != NULL)
        {
            pgO = GdiAllocBatchCommand(NULL, GdiBCDelObj);
            if (pgO)
            {
                /// FIXME: we need to mark the object as deleted!
                pgO->hgdiobj = hobj;
                return TRUE;
            }
        }
    }

    return NtGdiDeleteObjectApp(hobj);
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteObject(HGDIOBJ hObject)
{
    /* Check if the handle is valid (FIXME: we need some special
       sauce for the stock object flag) */
    if (!GdiValidateHandle(hObject))
        return FALSE;

    /* Check if this is a stock object */
    if ((DWORD_PTR)hObject & GDI_HANDLE_STOCK_MASK)
    {
        /* Ignore the attempt to delete a stock object */
        DPRINT1("Trying to delete system object 0x%p\n", hObject);
        return TRUE;
    }

    /* If we have any METAFILE objects, we need to check them */
    if (gcClientObj > 0)
    {
        DPRINT("Going Glue\n");
        METADC_RosGlueDeleteObject(hObject);
    }

    /* Switch by object type */
    switch (GDI_HANDLE_GET_TYPE(hObject))
    {
        case GDILoObjType_LO_METAFILE16_TYPE:
        case GDILoObjType_LO_METAFILE_TYPE:
            return FALSE;

        case GDILoObjType_LO_DC_TYPE:
        case GDILoObjType_LO_ALTDC_TYPE:
            return DeleteDC(hObject);

        case GDILoObjType_LO_ICMLCS_TYPE:
            return NtGdiDeleteColorSpace(hObject);

        case GDILoObjType_LO_REGION_TYPE:
            return DeleteRegion(hObject);

        case GDILoObjType_LO_BRUSH_TYPE:
        case GDILoObjType_LO_PEN_TYPE:
        case GDILoObjType_LO_EXTPEN_TYPE:
            return GdiDeleteBrushOrPen(hObject);

        case GDILoObjType_LO_FONT_TYPE:
        case GDILoObjType_LO_BITMAP_TYPE:
        default:
            break;
    }

    return NtGdiDeleteObjectApp(hObject);
}


