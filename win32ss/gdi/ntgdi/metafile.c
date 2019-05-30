/*
 * PROJECT:         ReactOS Win32k Subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/metafile.c
 * PURPOSE:         Metafile Implementations, Metafile Clipboard Data Xfers
 * PROGRAMMERS:     ...
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

// Need to move this to NtGdiTyp.h
#define GDITAG_TYPE_EMF 'XEFM' // EnhMetaFile
#define GDITAG_TYPE_MFP '_PFM' // MetaFile Picture

// Internal Use
typedef struct _METATYPEOBJ
{
    BASEOBJECT  BaseObject;
    DWORD iType;
    DWORD mm;
    DWORD xExt;
    DWORD yExt;
    ULONG cjData;
    PBYTE pjData[4];
} METATYPEOBJ, *PMETATYPEOBJ;


//
//  Plug Me in Somewhere? Clipboard cleanup?
//
VOID
FASTCALL
METATYPEOBJ__vCleanup(PVOID ObjectBody)
{
    PMETATYPEOBJ pmto = (PMETATYPEOBJ)ObjectBody;
    GDIOBJ_hInsertObject(&pmto->BaseObject, GDI_OBJ_HMGR_POWNED);
    GDIOBJ_vDeleteObject(&pmto->BaseObject);
}

/* System Service Calls ******************************************************/

/*
 * @implemented
 */
HDC
APIENTRY
NtGdiCreateMetafileDC(IN HDC hdc)
{
    //if (hdc)
        /* Call the internal function to create an alternative info DC */
        return GreCreateCompatibleDC(hdc, TRUE);
    // No HDC call NtUser.
    //return UserGetDesktopDC(DCTYPE_INFO, TRUE, FALSE);
}

/*
 * @implemented
 */
HANDLE
APIENTRY
NtGdiCreateServerMetaFile(
    IN DWORD iType,
    IN ULONG cjData,
    IN PBYTE pjData,
    IN DWORD mm,
    IN DWORD xExt,
    IN DWORD yExt
)
{
    BOOL Pass = TRUE;
    PMETATYPEOBJ pmto;

    if ( ( iType == GDITAG_TYPE_EMF || iType == GDITAG_TYPE_MFP ) &&
        cjData &&
        pjData )
    {
        pmto = (PMETATYPEOBJ)GDIOBJ_AllocObjWithHandle(GDIObjType_META_TYPE, sizeof(METATYPEOBJ) + cjData);
        if ( pmto )
        {
            pmto->iType  = iType;
            pmto->mm     = mm;
            pmto->xExt   = xExt;
            pmto->yExt   = yExt;
            pmto->cjData = cjData;

            _SEH2_TRY
            {
                 ProbeForRead( pjData, cjData, 1 );
                 RtlCopyMemory( pmto->pjData, pjData, cjData) ;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Pass = FALSE;
            }
            _SEH2_END;

            if (Pass)
            {
               GDIOBJ_vSetObjectOwner(&pmto->BaseObject, GDI_OBJ_HMGR_PUBLIC);
               GDIOBJ_vDereferenceObject ((POBJ)pmto);
               return pmto->BaseObject.hHmgr;
            }
            else
            {
               GDIOBJ_vDeleteObject(&pmto->BaseObject);
            }
        }
    }
    return NULL;
}

/*
 * @implemented
 */
ULONG
APIENTRY
NtGdiGetServerMetaFileBits(
    IN HANDLE hmo,
    IN ULONG cjData,
    OUT OPTIONAL PBYTE pjData,
    OUT PDWORD piType,
    OUT PDWORD pmm,
    OUT PDWORD pxExt,
    OUT PDWORD pyExt
)
{
    ULONG cjRet = 0;
    PMETATYPEOBJ pmto;

    pmto = (PMETATYPEOBJ) GDIOBJ_ShareLockObj ((HGDIOBJ) hmo, GDIObjType_META_TYPE);

    if (!pmto)
        return 0;

    if ( pmto->iType == GDITAG_TYPE_EMF || pmto->iType == GDITAG_TYPE_MFP )
    {
        cjRet = pmto->cjData;

        if ( cjData )
        {
            if (cjData == pmto->cjData)
            {
                _SEH2_TRY
                {
                    ProbeForWrite( piType, sizeof(DWORD), 1);
                    *piType = pmto->iType;

                    ProbeForWrite( pmm, sizeof(DWORD), 1);
                    *pmm = pmto->mm;

                    ProbeForWrite( pxExt, sizeof(DWORD), 1);
                    *pxExt = pmto->xExt;

                    ProbeForWrite( pyExt, sizeof(DWORD), 1);
                    *pyExt = pmto->yExt;

                    ProbeForWrite( pjData, cjData, 1 );
                    RtlCopyMemory( pjData, pmto->pjData, cjData) ;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    cjRet = 0;
                }
                _SEH2_END;
            }
            else
            {
                cjRet = 0;
            }
        }
    }

    GDIOBJ_vDereferenceObject ((POBJ)pmto);
    return cjRet;
}

/*
 * @unimplemented
 */
LONG
APIENTRY
NtGdiConvertMetafileRect(IN HDC hDC,
                         IN OUT PRECTL pRect)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
