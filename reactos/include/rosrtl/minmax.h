/* $Id: minmax.h,v 1.1 2003/11/17 02:12:52 hyperion Exp $
 */

#ifndef ROSRTL_MINMAX_H__
#define ROSRTL_MINMAX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define RtlRosMin(X,Y) (((X) < (Y))? (X) : (Y))
#define RtlRosMax(X,Y) (((X) > (Y))? (X) : (Y))
#define RtlRosMin3(X,Y,Z) (((X) < (Y)) ? RtlRosMin(X,Z) : RtlRosMin(Y,Z))
#define RtlRosMax3(X,Y,Z) (((X) > (Y)) ? RtlRosMax(X,Z) : RtlRosMax(Y,Z))

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
