#ifndef __FT_STROKER_H__
#define __FT_STROKER_H__

#include <ft2build.h>
#include FT_OUTLINE_H

FT_BEGIN_HEADER

/*@*************************************************************
 *
 * @type: FT_Stroker
 *
 * @description:
 *    opaque handler to a path stroker object
 */
  typedef struct FT_StrokerRec_*    FT_Stroker;


/*@*************************************************************
 *
 * @enum: FT_Stroker_LineJoin
 *
 * @description:
 *    these values determine how two joining lines are rendered
 *    in a stroker.
 *
 * @values:
 *    FT_STROKER_LINEJOIN_ROUND ::
 *      used to render rounded line joins. circular arcs are used
 *      to join two lines smoothly
 *
 *    FT_STROKER_LINEJOIN_BEVEL ::
 *      used to render beveled line joins; i.e. the two joining lines
 *      are extended until they intersect
 *
 *    FT_STROKER_LINEJOIN_MITER ::
 *      same as beveled rendering, except that an additional line
 *      break is added if the angle between the two joining lines
 *      is too closed (this is useful to avoid unpleasant spikes
 *      in beveled rendering).
 */
  typedef enum
  {
    FT_STROKER_LINEJOIN_ROUND = 0,
    FT_STROKER_LINEJOIN_BEVEL,
    FT_STROKER_LINEJOIN_MITER

  } FT_Stroker_LineJoin;


/*@*************************************************************
 *
 * @enum: FT_Stroker_LineCap
 *
 * @description:
 *    these values determine how the end of opened sub-paths are
 *    rendered in a stroke
 *
 * @values:
 *    FT_STROKER_LINECAP_BUTT ::
 *      the end of lines is rendered as a full stop on the last
 *      point itself
 *
 *    FT_STROKER_LINECAP_ROUND ::
 *      the end of lines is rendered as a half-circle around the
 *      last point
 *
 *    FT_STROKER_LINECAP_SQUARE ::
 *      the end of lines is rendered as a square around the
 *      last point
 */
  typedef enum
  {
    FT_STROKER_LINECAP_BUTT = 0,
    FT_STROKER_LINECAP_ROUND,
    FT_STROKER_LINECAP_SQUARE

  } FT_Stroker_LineCap;

 /* */

  FT_EXPORT( FT_Error )
  FT_Stroker_New( FT_Memory    memory,
                  FT_Stroker  *astroker );

  FT_EXPORT( void )
  FT_Stroker_Set( FT_Stroker           stroker,
                  FT_Fixed             radius,
                  FT_Stroker_LineCap   line_cap,
                  FT_Stroker_LineJoin  line_join,
                  FT_Fixed             miter_limit );


  FT_EXPORT( FT_Error )
  FT_Stroker_ParseOutline( FT_Stroker   stroker,
                           FT_Outline*  outline,
                           FT_Bool      opened );

  FT_EXPORT( FT_Error )
  FT_Stroker_BeginSubPath( FT_Stroker  stroker,
                           FT_Vector*  to,
                           FT_Bool     open );

  FT_EXPORT( FT_Error )
  FT_Stroker_EndSubPath( FT_Stroker  stroker );


  FT_EXPORT( FT_Error )
  FT_Stroker_LineTo( FT_Stroker  stroker,
                     FT_Vector*  to );

  FT_EXPORT( FT_Error )
  FT_Stroker_ConicTo( FT_Stroker  stroker,
                      FT_Vector*  control,
                      FT_Vector*  to );

  FT_EXPORT( FT_Error )
  FT_Stroker_CubicTo( FT_Stroker  stroker,
                      FT_Vector*  control1,
                      FT_Vector*  control2,
                      FT_Vector*  to );


  FT_EXPORT( FT_Error )
  FT_Stroker_GetCounts( FT_Stroker  stroker,
                        FT_UInt    *anum_points,
                        FT_UInt    *anum_contours );

  FT_EXPORT( void )
  FT_Stroker_Export( FT_Stroker   stroker,
                     FT_Outline*  outline );

  FT_EXPORT( void )
  FT_Stroker_Done( FT_Stroker  stroker );


FT_END_HEADER

#endif /* __FT_STROKER_H__ */
