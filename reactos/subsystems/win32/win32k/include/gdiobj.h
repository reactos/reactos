#ifndef __WIN32K_GDIOBJ_H
#define __WIN32K_GDIOBJ_H


typedef struct tagGDIOBJHDR
{
    SHORT        type;         /* object type (one of the OBJ_* constants) */
    SHORT        system : 1;   /* system object flag */
    SHORT        deleted : 1;  /* whether DeleteObject has been called on this object */
    LONG         selcount;     /* number of times the object is selected in a DC */
} GDIOBJHDR;

extern BOOLEAN GDIOBJ_Init(void);
extern HGDIOBJ alloc_gdi_handle( GDIOBJHDR *obj, SHORT type);
extern void *free_gdi_handle( HGDIOBJ handle );
extern void *GDI_GetObjPtr( HGDIOBJ, SHORT );
extern void GDI_ReleaseObj( HGDIOBJ );
extern HGDIOBJ GDI_inc_ref_count( HGDIOBJ handle );
extern BOOL GDI_dec_ref_count( HGDIOBJ handle );

/* Handle mapping */
VOID NTAPI GDI_InitHandleMapping();
VOID NTAPI GDI_AddHandleMapping(HGDIOBJ hKernel, HGDIOBJ hUser);
HGDIOBJ NTAPI GDI_MapUserHandle(HGDIOBJ hUser);
VOID NTAPI GDI_RemoveHandleMapping(HGDIOBJ hUser);

#endif
