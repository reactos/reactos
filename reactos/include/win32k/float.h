typedef struct tagFLOAT_POINT
{
   FLOAT x, y;
} FLOAT_POINT;

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static inline void INTERNAL_LPTODP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;
    
    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->w.xformWorld2Vport.eM11 +
               y * dc->w.xformWorld2Vport.eM21 +
	       dc->w.xformWorld2Vport.eDx;
    point->y = x * dc->w.xformWorld2Vport.eM12 +
               y * dc->w.xformWorld2Vport.eM22 +
	       dc->w.xformWorld2Vport.eDy;
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in integer format). Returns TRUE if successful, else FALSE.
 */
static inline BOOL INTERNAL_DPTOLP(DC *dc, LPPOINT point)
{
    FLOAT_POINT floatPoint;
    
    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    if (!INTERNAL_DPTOLP_FLOAT(dc, &floatPoint))
        return FALSE;
    
    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);

    return TRUE;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in integer format).
 */
static inline void INTERNAL_LPTODP(DC *dc, LPPOINT point)
{
    FLOAT_POINT floatPoint;
    
    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    INTERNAL_LPTODP_FLOAT(dc, &floatPoint);
    
    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);
}

#define XDPTOLP(dc,x) \
    (MulDiv(((x)-(dc)->vportOrgX), (dc)->wndExtX, (dc)->vportExtX) + (dc)->wndOrgX)
#define YDPTOLP(dc,y) \
    (MulDiv(((y)-(dc)->vportOrgY), (dc)->wndExtY, (dc)->vportExtY) + (dc)->wndOrgY)
#define XLPTODP(dc,x) \
    (MulDiv(((x)-(dc)->wndOrgX), (dc)->vportExtX, (dc)->wndExtX) + (dc)->vportOrgX)
#define YLPTODP(dc,y) \
    (MulDiv(((y)-(dc)->wndOrgY), (dc)->vportExtY, (dc)->wndExtY) + (dc)->vportOrgY)

  /* Device <-> logical size conversion */

#define XDSTOLS(dc,x) \
    MulDiv((x), (dc)->wndExtX, (dc)->vportExtX)
#define YDSTOLS(dc,y) \
    MulDiv((y), (dc)->wndExtY, (dc)->vportExtY)
#define XLSTODS(dc,x) \
    MulDiv((x), (dc)->vportExtX, (dc)->wndExtX)
#define YLSTODS(dc,y) \
    MulDiv((y), (dc)->vportExtY, (dc)->wndExtY)
